//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

namespace Async
{
    namespace Details
    {
        // Helper class for `RunOnBackground` function.
        template <typename FuncType>
        struct RunOnBackgroundContext
        {
            FuncType Func;

            static void CALLBACK Callback(
                _Inout_ PTP_CALLBACK_INSTANCE Instance,
                _Inout_opt_ void* Context)
            {
                (void)Instance;
                std::unique_ptr<RunOnBackgroundContext> context(reinterpret_cast<RunOnBackgroundContext*>(Context));
                context->Func();
            }
        };
    }

    // Runs a functor on a threadpool thread.
    template <typename FuncType>
    inline void RunOnBackground(FuncType&& func)
    {
        using ContextType = Details::RunOnBackgroundContext<std::remove_reference_t<FuncType>>;
        auto context = std::unique_ptr<ContextType>(new ContextType{ std::forward<FuncType>(func) });

        BOOL submitResult = TrySubmitThreadpoolCallback(&ContextType::Callback, context.get(), nullptr);
        if (!submitResult)
        {
            throw std::system_error(::GetLastError(), std::system_category());
        }

        // Transfer ownership of context to callback.
        context.release();
    }

    namespace Details
    {
        // Stores the result value for `AsyncTaskBase`.
        // This type exists to help handle the case where the result type is `void`.
        template <typename ResultType>
        struct AsyncTaskBaseStorage
        {
            template <typename... ArgTypes>
            AsyncTaskBaseStorage(ArgTypes&&... args) :
                _Value{ std::forward<ArgTypes>(args)... }
            {
            }

            ResultType Acquire()
            {
                return std::move(_Value);
            }

        private:
            ResultType _Value;
        };

        template <>
        struct AsyncTaskBaseStorage<void>
        {
            void Acquire()
            {
            }
        };
    }

    // An object that returns a result sometime in the future. (e.g. std::future, IAsyncOperation).
    template <typename ResultType>
    class AsyncTaskBase :
        public std::enable_shared_from_this<AsyncTaskBase<ResultType>>
    {
    public:
        // The function signature of the completed callback.
        //
        // Note: The task's result can be retrieved at any time after the task has completed, not just during
        // the completed callback.
        using CompletedHandlerType = void(AsyncTaskBase<ResultType>& asyncTask);

        // Sets a callback function that is called when the task completes.
        // If the task has already completed, then the callback is invoked during the SetCompletedHandler call.
        //
        // Return:
        //   true - The callback function was succesfully set.
        //   false - The callback function has already been set.
        bool SetCompletedHandler(std::function<CompletedHandlerType>&& completedHandler)
        {
            std::unique_lock<std::mutex> gate(_Lock);
            if (_CompletedHandlerWasSet)
            {
                // Completed handler already set.
                return false;
            }

            if (!_Result.has_value())
            {
                // Set the completed handler.
                _CompletedHandler = std::move(completedHandler);
                _CompletedHandlerWasSet = true;
                return true;
            }

            _CompletedHandlerWasSet = true;
            gate.unlock();

            // Both result and completed handler are now available.
            // So call the completed handler immediately.
            completedHandler(*this);
            return true;
        }

        // Sets a callback function that is called when the task completes.
        // The callback is guaranteed not to be called during the SetAsyncCompletedHandler call.
        //
        // Return:
        //   true - The callback function was succesfully set.
        //   false - The callback function has already been set.
        bool SetAsyncCompletedHandler(std::function<CompletedHandlerType>&& completedHandler)
        {
            std::unique_lock<std::mutex> gate(_Lock);
            if (_CompletedHandlerWasSet)
            {
                // Completed handler already set.
                return false;
            }

            // Set the completed handler.
            _CompletedHandler = std::move(completedHandler);
            _CompletedHandlerWasSet = true;

            if (!_Result.has_value())
            {
                // No result available yet. The callback will be invoked when the result is provided.
                return true;
            }

            gate.unlock();

            // Both result and completed handler are now available.
            // Queue the completed callback to be invoked on another thread.
            RunOnBackground(
                [me = shared_from_this()]()
            {
                // Note: It is safe to read and modify _CompletedHandler here outside of the lock as
                //   no other thread will ever touch the variable again.
                me->_CompletedHandler(*me);
                me->_CompletedHandler = nullptr;
            });

            return true;
        }

        // Returns if the task has completed.
        bool Completed()
        {
            std::unique_lock<std::mutex> gate(_Lock);
            return _Result.has_value();
        }

        // Requests that the async operation is canceled.
        // This is best effort and may be ignored.
        void Cancel()
        {
            std::unique_lock<std::mutex> gate(_Lock);

            if (_Canceled || _Result.has_value())
            {
                // Task has already been canceled or the task has already completed.
                return;
            }

            _Canceled = true;

            gate.unlock();

            // Tell child class that a cancelation request has been made.
            OnCanceled();
        }

        // Waits for the task to complete, with a timeout.
        //
        // Return:
        //   true - The task completed succesfully.
        //   false - The wait timed out.
        bool Wait(DWORD timeoutMilliseconds)
        {
            std::unique_lock<std::mutex> gate(_Lock);
            while (!_Result.has_value())
            {
                if (std::cv_status::timeout == _Updated.wait_for(gate, std::chrono::milliseconds(timeoutMilliseconds)))
                {
                    return false;
                }
            }

            return true;
        }

        // Waits for the task to complete.
        void Wait()
        {
            std::unique_lock<std::mutex> gate(_Lock);
            while (!_Result.has_value())
            {
                _Updated.wait(gate);
            }
        }

        // Waits and returns the result (via std::move).
        ResultType Get()
        {
            std::unique_lock<std::mutex> gate(_Lock);
            while (!_Result.has_value())
            {
                _Updated.wait(gate);
            }

            return _Result.value().Acquire();
        }

    protected:
        // Returns if a cancelation has been requested.
        bool Canceled()
        {
            std::unique_lock<std::mutex> gate(_Lock);
            return _Canceled;
        }

        // Set the result of the task.
        //
        // Return:
        //   true - The result was succesfully set.
        //   false - The result has already been set.
        template <typename... ArgTypes>
        bool EmplaceResult(ArgTypes&&... args)
        {
            std::unique_lock<std::mutex> gate(_Lock);
            if (_Result.has_value())
            {
                return false;
            }

            _Result.emplace(std::forward<ArgTypes>(args)...);
            _Updated.notify_all();

            if (!_CompletedHandlerWasSet)
            {
                // Still waiting on the completed handler.
                return true;
            }

            gate.unlock();

            // Both result and completed handler are now available.
            // So call the completed handler.
            // Note: It is safe to read and modify _CompletedHandler here outside of the lock as
            //   no other thread will ever touch the variable again.
            _CompletedHandler(*this);
            _CompletedHandler = nullptr;
            return true;
        }

    protected:
        virtual void OnCanceled()
        {
        }

    private:
        std::mutex _Lock;
        std::condition_variable _Updated;
        std::function<CompletedHandlerType> _CompletedHandler;
        std::optional<Details::AsyncTaskBaseStorage<ResultType>> _Result;
        bool _CompletedHandlerWasSet = false;
        bool _Canceled = false;
    };

    // A type that promises to provide a value sometime in the future. (e.g. std::promise)
    template <typename ResultType>
    class AsyncTaskCompletionSource final :
        public AsyncTaskBase<ResultType>
    {
    public:
        AsyncTaskCompletionSource() = default;
        using AsyncTaskBase::EmplaceResult;
        using AsyncTaskBase::Canceled;
    };

    // Makes a `AsyncTaskCompletionSource` type.
    template <typename ResultType>
    std::shared_ptr<AsyncTaskCompletionSource<ResultType>> MakeCompletionSource()
    {
        return std::make_shared<AsyncTaskCompletionSource<ResultType>>();
    }

    namespace Details
    {
        // Implementation of `RunBackgroundTask` function, when the result type is not `void`.
        template <bool IsResultVoid>
        struct RunBackgroundTaskImpl
        {
            template <typename FuncType>
            static auto Run(FuncType&& func) -> std::shared_ptr<AsyncTaskBase<std::invoke_result_t<FuncType>>>
            {
                using ResultType = std::invoke_result_t<FuncType>;
                std::shared_ptr<AsyncTaskCompletionSource<ResultType>> completionSource = MakeCompletionSource<ResultType>();

                RunOnBackground(
                    [func = std::forward<FuncType>(func), completionSource]()
                {
                    completionSource->EmplaceResult(func());
                });

                return completionSource;
            }
        };

        // Implementation of `RunBackgroundTask` function, when the result type is `void`.
        template <>
        struct RunBackgroundTaskImpl<true>
        {
            template <typename FuncType>
            static auto Run(FuncType&& func) -> std::shared_ptr<AsyncTaskBase<std::invoke_result_t<FuncType>>>
            {
                using ResultType = std::invoke_result_t<FuncType>;
                std::shared_ptr<AsyncTaskCompletionSource<ResultType>> completionSource = MakeCompletionSource<ResultType>();

                RunOnBackground(
                    [func = std::forward<FuncType>(func), completionSource]()
                {
                    func();
                    completionSource->EmplaceResult();
                });

                return completionSource;
            }
        };
    }

    // Executes a function on a threadpool thread.
    //
    // Return: A task that completes when the function has finished executing, which provides the result of the function.
    template <typename FuncType>
    inline auto RunBackgroundTask(FuncType&& func) -> std::shared_ptr<AsyncTaskBase<std::invoke_result_t<FuncType>>>
    {
        using ImplType = Details::RunBackgroundTaskImpl<std::is_void_v<std::invoke_result_t<FuncType>>>;
        return ImplType::Run(std::forward<FuncType>(func));
    }

    namespace Details
    {
        // The type that keeps track of the state of a `WaitForAny` call.
        struct WaitForAnyContext
        {
            static constexpr size_t UnsetTaskIndex = 0;
            std::atomic<size_t> CompletedTaskIndex = UnsetTaskIndex;
        };

        // Sets the completed handler on the provided task for the WaitForAnyFor function.
        template <typename ResultType>
        inline void WaitForAny_SetCompletedHandler(size_t index, const std::shared_ptr<WaitForAnyContext>& context, AsyncTaskBase<ResultType>& task)
        {
            task.SetCompletedHandler(
                [index, context](AsyncTaskBase<ResultType>& task)
            {
                (void)task;
                size_t expectedValue = WaitForAnyContext::UnsetTaskIndex;
                if (context->CompletedTaskIndex.compare_exchange_strong(expectedValue, index))
                {
                    ::WakeByAddressAll(&context->CompletedTaskIndex);
                }
            });
        }

        // Sets the completed handler on the provided tasks for the WaitForAnyFor function.
        template <typename ResultType, typename... ResultTypes>
        inline void WaitForAny_SetCompletedHandler(size_t index, const std::shared_ptr<WaitForAnyContext>& context, AsyncTaskBase<ResultType>& task, AsyncTaskBase<ResultTypes>&... tasks)
        {
            WaitForAny_SetCompletedHandler(index, context, task);
            WaitForAny_SetCompletedHandler(index + 1, context, tasks...);
        }
    }

    // Waits for any of the provided tasks to complete or for the timeout to expire.
    //
    // Return:
    //   0 - The timeout expired.
    //   1+ - The index of the task that completed.
    template <typename... ResultTypes>
    inline size_t WaitForAnyWithTimeout(DWORD timeoutMilliseconds, const std::shared_ptr<AsyncTaskBase<ResultTypes>>&... tasks)
    {
        auto context = std::make_shared<Details::WaitForAnyContext>();
        Details::WaitForAny_SetCompletedHandler(1, context, (*tasks)...);

        size_t index = context->CompletedTaskIndex;
        while (index == Details::WaitForAnyContext::UnsetTaskIndex)
        {
            static_assert(sizeof(index) == sizeof(context->CompletedTaskIndex), "Unsupported std::atomic implementation");
            if (!::WaitOnAddress(&context->CompletedTaskIndex, &index, sizeof(index), timeoutMilliseconds))
            {
                // Timed out.
                return 0;
            }

            index = context->CompletedTaskIndex;
        }

        return index;
    }

    // Waits for any of the provided tasks to complete.
    //
    // Return: The index of the task that completed.
    template <typename... ResultTypes>
    inline size_t WaitForAny(const std::shared_ptr<AsyncTaskBase<ResultTypes>>&... tasks)
    {
        size_t index = WaitForAnyWithTimeout(INFINITE, tasks...);
        return index - 1;
    }
}
