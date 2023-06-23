OBS Studio Frontend Broadcast Flow API
======================================

The OBS Studio frontend broadcast flow API is part of the frontend API which
can enable a deeper integration for streaming services plugin.

Services usually does not separate the concept of broadcast and stream, but
some does. In this case features related to differed start/stop might need to be
enabled.

NOTE: If the service has its bandwidth test enabled, the broadcast flow is ignored.

.. code:: cpp

   #include <obs-frontend-api.h>


Structures/Enumerations
-----------------------

.. struct:: obs_frontend_broadcast_flow

   Broadcast Flow definition structure.

  .. member:: uint32_t obs_frontend_broadcast_flow.flags

     Broadcast Flow feature flags.

     A bitwise OR combination of one or more of the following values:

     - **OBS_BROADCAST_FLOW_ALLOW_MANAGE_WHILE_STREAMING** - Allow broadcast
       management while streaming.

     - **OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_START** - Broadcast can be
       started after the stream.

     - **OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_STOP** - Broadcast can be
       stopped after the stream.

  .. member:: obs_broadcast_state (*obs_frontend_broadcast_flow.get_broadcast_state)(void *priv)

     Return the state of the broadcast. Called when the flow is added, after all
     other callbacks except manage while streaming, stream state and last error.

     (Required)

     :param  priv:  The priv variable of this structure
     :return:       The state of the broadcast

  .. member:: obs_broadcast_start (*obs_frontend_broadcast_flow.get_broadcast_start_type)(void *priv)

     Return the start type of the broadcast. Called when the flow is added, after
     manage (while not streaming), stopped streaming and differed stop (if
     OBS_BROADCAST_STOP_DIFFER_FROM_STREAM) callbacks.

     (Required)

     :param  priv:  The priv variable of this structure
     :return:       The start type of the broadcast

  .. member:: obs_broadcast_stop (*obs_frontend_broadcast_flow.get_broadcast_stop_type)(void *priv)

     Return the stop type of the broadcast. Called when the flow is added, after
     manage (while not streaming), stopped streaming and differed stop (if
     OBS_BROADCAST_STOP_DIFFER_FROM_STREAM) callbacks.

     (Required)

     :param  priv:  The priv variable of this structure
     :return:       The stop type of the broadcast

  .. member:: void (*obs_frontend_broadcast_flow.manage_broadcast)(void *priv)

     Show the broadcast manager of the service.

     (Require if not OBS_BROADCAST_FLOW_ALLOW_MANAGE_WHILE_STREAMING)

     :param  priv:  The priv variable of this structure

  .. member:: void (*obs_frontend_broadcast_flow.manage_broadcast2)(void *priv, bool streaming_active)

     Show the broadcast manager of the service.

     (Require if OBS_BROADCAST_FLOW_ALLOW_MANAGE_WHILE_STREAMING)

     :param  priv:  The priv variable of this structure

  .. member:: void (*obs_frontend_broadcast_flow.stopped_streaming)(void *priv)

     Indicate to the broadcast flow that the stream is terminated.

     It allow to update the state of the broadcast before being requested by the UI.

     (Required)

     :param  priv:  The priv variable of this structure

  .. member:: void (*obs_frontend_broadcast_flow.differed_start_broadcast)(void *priv)

     Make the broadcast start and must change the state to active if it succeed.

     A thread with calling the following member will be started to
     check if the stream is active.

     (Required if OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_START)

     :param  priv:  The priv variable of this structure

  .. member:: enum obs_broadcast_stream_state (*obs_frontend_broadcast_flow.is_broadcast_stream_active)(void *priv)

     This member will be called in thread to check if the stream has started.

     The thread will timeout if too many inactive state are returned.

     (Required if OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_START)

     :param  priv:  The priv variable of this structure
     :return:       The state of the broadcast stream

  .. member:: bool (*obs_frontend_broadcast_flow.differed_stop_broadcast)(void *priv)

     Make the broadcast stop.

     (Required if OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_STOP)

     :param  priv:  The priv variable of this structure
     :return:       If the broadcast was successfully ended

  .. member:: const char *(*obs_frontend_broadcast_flow.get_last_error)(void *priv)

     If differed start/stop failed, this member can return the error.

     (Required if OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_START or
     OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_STOP)

     :param  priv:  The priv variable of this structure

---------------------------------------

.. enum:: obs_broadcast_state

   - **OBS_BROADCAST_NONE**

     No broadcast is setup.

   - **OBS_BROADCAST_ACTIVE**

     The broadcast is ready and active.

   - **OBS_BROADCAST_INACTIVE**

     The broadcast is ready and awaits a differed start.
     Ignored if not OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_START.

.. enum:: obs_broadcast_start

   - **OBS_BROADCAST_START_WITH_STREAM**

     The broadcast is already started or will start with the stream.

   - **OBS_BROADCAST_START_WITH_STREAM_NOW**

     Same as the previous, but the streaming will also be started.

   - **OBS_BROADCAST_START_DIFFER_FROM_STREAM**

     The broadcast requires to be started after the stream.
     Ignored if not OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_START.

---------------------------------------

.. enum:: obs_broadcast_stop

   - **OBS_BROADCAST_STOP_NEVER**

     The broadcast is not stopped after the stream ended.

   - **OBS_BROADCAST_STOP_WITH_STREAM**

     The broadcast is ended at the time as the stream.

   - **OBS_BROADCAST_STOP_DIFFER_FROM_STREAM**

     The broadcast requires to be stopped separetly from the stream.
     Ignored if not OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_STOP.

---------------------------------------

.. enum:: obs_broadcast_stream_state

   - **OBS_BROADCAST_STREAM_FAILURE**

     The broadcast stream could not be activated.

   - **OBS_BROADCAST_STREAM_INACTIVE**

     The broadcast stream is inactive.

   - **OBS_BROADCAST_STREAM_ACTIVE**

     The broadcast stream is active.

---------------------------------------

Functions
---------

.. function:: void obs_frontend_add_broadcast_flow(const obs_service_t *service, const struct obs_frontend_broadcast_flow *flow)

   Add a Broadcast Flow bounded to the given service.

   :param service: The service bounded the the flow
   :param flow: The flow itself

---------------------------------------

.. function:: void obs_frontend_remove_broadcast_flow(const obs_service_t *service)

   Remove the Broadcast Flow bounded to the given service.

   :param service: The service bounded the the flow
