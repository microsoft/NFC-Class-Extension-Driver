# NfcCx and State Machines

## The LibNfc thread

Within the NfcCx, there is a single thread that processes all communication between the driver and the hardware. This thread is used to serialize all hardware operations (and avoid requiring other concurrency primitives such as locks).

Colloquially this thread is known as the LibNfc thread. Though the thread is owned by the NfcCxRf class and its code lives in the OSAL (OS abstraction library) library.

Unfortunately, the NFC Controller is a shared resource that can have multiple operations running at the same time. So all operations had to be written in an asynchronous manner, to allow them to be interwoven on the single LibNfc thread.

At the fundamental level, the asynchronous programming pattern used is the command/callback pattern. But two abstractions were added on top of this in attempt to make the code more declarative: state handling and sequence handling.

## State handling

The state machines in NfcCx include:

1. NCI core (send and receive):
    - \libs\NfcCoreLib\lib\NciCore\phNciNfc_CoreSend.c
    - \libs\NfcCoreLib\lib\NciCore\phNciNfc_CoreRecv.c
2. LibNfc:
    - \libs\NfcCoreLib\lib\LibNfc\phLibNfc_State.c
3. NfcCxRf (NfcCxState):
    - \Cx\NfcCxState.cpp

These four state machines could theoretically be considered part of a single [hierarchical discrete state machine](https://en.wikipedia.org/wiki/UML_state_machine#Hierarchically_nested_states). However there is no direct relationship between them in the actual code. Instead each state machine is used as an implementation detail for the library it is used in (i.e. CX, LibNfc, NCI Core). And each library uses the traditional command/callback asynchronous pattern in its API.

An example of a LibNfc's API:

```cpp
// Callback type
typedef void (*pphLibNfc_TransceiveCallback_t)(void* pContext, phLibNfc_Handle hRemoteDev,
    phNfc_sData_t* pResBuffer, NFCSTATUS Status);

// Asynchronous command that returns its results through a callback.
NFCSTATUS phLibNfc_RemoteDev_Transceive(phLibNfc_Handle hRemoteDevice, phLibNfc_sTransceiveInfo_t* psTransceiveInfo,
    pphLibNfc_TransceiveCallback_t pTransceive_RspCb, void* pContext);
```

### State machines' implementation differences

The four state machines all have different implementations, with the `NfcCxState` implementation being radically different from the other three. But all of them roughly follow the same guiding principles.

### Events

Within the state machine, an event is something that triggers a state transition to occur.

For example, in `NfcCxState`:

```text
                   NfcCxEventInit                       NfcCxEventReqCompleted
NfcCxStateIdle ----------------------> NfcCxStateInit --------------------------> StateRfIdle
                |
                |  NfcCxEventFailed
                ---------------------> NfcCxStateShutdown
```

### Chaining events

In the above example, the `NfcCxEventReqCompleted` and `NfcCxEventFailed` events represent the end of the initialize operation. So if there is something waiting for the initialize operation to complete, it needs to wait for both the `NfcCxEventInit` state transition and either the `NfcCxEventReqCompleted` or the `NfcCxEventFailed` state transition.

Within `NfcCxState`, to make tracking the end of an operation "easier", the state machine code serves a double purpose of both managing the state machine and tracking the currently running operation. However within the LibNfc and NCI Core libraries, the sequence handling code is responsible for signaling the end of an operation.

### User events

Within `NfcCxState`, a distinction is made between user events and internal events. User events are used where another thread is waiting for the operation to complete. Internal events are used for all other operations.

### Pseudo events

Some operations (such as eSE APDU transmit) don't change the state of the state machine. However such operations usually require the state machine to be in a particular state to execute and need to prevent a state transition from occurring while they are running. These requirements are almost identical to the requirements of a state transition operation. That is, a state transition must start in a particular state and can't be run concurrently with another state transition.

So normal operations re-use the code intended for state transition operations but using a pseudo event. (Though at the cost of making the code somewhat less intuitive.)

The pseudo events include:

- In LibNfc:
    - `phLibNfc_EventDummy`
- In `NfcCxState`:
    - `NfcCxEventDataXchg`
    - `NfcCxEventSE`
    - `NfcCxEventConfig`

For example, the eSE APDU transmit operation results in the following state transitions:

```text
                   NfcCxEventSE (transmit)                       NfcCxEventReqCompleted
NfcCxStateRfIdle ---------------------------> NfcCxStateRfIdle --------------------------> NfcCxStateRfIdle
```

### Connector states

Some events can result in one of a number of state transitions based on some external condition. To represent these dynamic state transitions a pseudo state, known as a connector state, is used. When the state machine encounters a connector state, it invokes the relevant function to evaluate some conditional. The result of this evaluation is then used to map to the new state.

For example, in `NfcCxState`, the `NfcCxStateConnChkDiscMode` state is used when the tag discovered operation fails:

```text
                        NfcCxEventDiscovered                             NfcCxEventActivated
NfcCxStateRfDiscovery ------------------------> NfcCxStateRfDiscovered -----------------------> NfcCxStateRfDataXchg
                                                         |
                                                         |
                                                         |  NfcCxEventDeactivated
                                                         |
                                                         |
                                               NfcCxStateConnChkDiscMode
                                                         |       \
                                                         |        \ Enable
                                                 Disable |         \ Discovery
                                               Discovery |          \
                                                         |           -------------> NfcCxStateRfDiscovery
                                                         |
                                                         -------------------------> NfcCxStateRfIdle
```
