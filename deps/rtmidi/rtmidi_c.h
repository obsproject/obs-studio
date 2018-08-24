
#include <stdbool.h>
#include <stddef.h>
#ifndef RTMIDI_C_H
#define RTMIDI_C_H

#if defined(RTMIDI_EXPORT)
#define RTMIDIAPI __declspec(dllexport)
#else
#define RTMIDIAPI //__declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

//! Wraps an RtMidi object for C function return statuses.
struct RtMidiWrapper {
    //! The wrapped RtMidi object.
    void* ptr;
    void* data;

    //! True when the last function call was OK. 
    bool  ok;

    //! If an error occured (ok != true), set to an error message.
    const char* msg;
};

//! Typedef for a generic RtMidi pointer.
typedef RtMidiWrapper* RtMidiPtr;

//! Typedef for a generic RtMidiIn pointer.
typedef RtMidiWrapper* RtMidiInPtr;

//! Typedef for a generic RtMidiOut pointer.
typedef RtMidiWrapper* RtMidiOutPtr;


enum RtMidiApi {
    RT_MIDI_API_UNSPECIFIED,    /*!< Search for a working compiled API. */
    RT_MIDI_API_MACOSX_CORE,    /*!< Macintosh OS-X Core Midi API. */
    RT_MIDI_API_LINUX_ALSA,     /*!< The Advanced Linux Sound Architecture API. */
    RT_MIDI_API_UNIX_JACK,      /*!< The Jack Low-Latency MIDI Server API. */
    RT_MIDI_API_WINDOWS_MM,     /*!< The Microsoft Multimedia MIDI API. */
    RT_MIDI_API_WINDOWS_KS,     /*!< The Microsoft Kernel Streaming MIDI API. */
    RT_MIDI_API_RTMIDI_DUMMY    /*!< A compilable but non-functional API. */
  };

enum RtMidiErrorType {
  RT_ERROR_WARNING, RT_ERROR_DEBUG_WARNING, RT_ERROR_UNSPECIFIED, RT_ERROR_NO_DEVICES_FOUND,
  RT_ERROR_INVALID_DEVICE, RT_ERROR_MEMORY_ERROR, RT_ERROR_INVALID_PARAMETER, RT_ERROR_INVALID_USE,
  RT_ERROR_DRIVER_ERROR, RT_ERROR_SYSTEM_ERROR, RT_ERROR_THREAD_ERROR
};

/*! The type of a RtMidi callback function.
 * \param timeStamp   The time at which the message has been received.
 * \param message     The midi message.
 * \param userData    Additional user data for the callback.
 */
typedef void(* RtMidiCCallback) (double timeStamp, const unsigned char* message, void *userData);

//! Returns the size (with sizeof) of a RtMidiApi instance.
RTMIDIAPI int rtmidi_sizeof_rtmidi_api ();

/* RtMidi API */

/*! Determine the available compiled MIDI APIs.
 * If the given `apis` parameter is null, returns the number of available APIs.
 * Otherwise, fill the given apis array with the RtMidi::Api values.
 *
 * \param apis  An array or a null value.
*/
RTMIDIAPI int rtmidi_get_compiled_api (enum RtMidiApi **apis); // return length for NULL argument.

//! Report an error.
RTMIDIAPI void rtmidi_error (enum RtMidiErrorType type, const char* errorString);

/*! Open a MIDI port.  
 *
 * \param port      Must be greater than 0
 * \param portName  Name for the application port.
 */
RTMIDIAPI void rtmidi_open_port (RtMidiPtr device, unsigned int portNumber, const char *portName);

/*! Creates a virtual MIDI port to which other software applications can 
 * connect.  
 *
 * \param portName  Name for the application port.
 */
RTMIDIAPI void rtmidi_open_virtual_port (RtMidiPtr device, const char *portName);

/*! Close a MIDI connection.
 */
RTMIDIAPI void rtmidi_close_port (RtMidiPtr device);

/*! Return the number of available MIDI ports.
 */
RTMIDIAPI unsigned int rtmidi_get_port_count (RtMidiPtr device);

/*! Return a string identifier for the specified MIDI input port number.
 */
RTMIDIAPI const char* rtmidi_get_port_name (RtMidiPtr device, unsigned int portNumber);

/* RtMidiIn API */

//! Create a default RtMidiInPtr value, with no initialization.
RTMIDIAPI RtMidiInPtr rtmidi_in_create_default ();

/*! Create a  RtMidiInPtr value, with given api, clientName and queueSizeLimit.
 *
 *  \param api            An optional API id can be specified.
 *  \param clientName     An optional client name can be specified. This
 *                        will be used to group the ports that are created
 *                        by the application.
 *  \param queueSizeLimit An optional size of the MIDI input queue can be
 *                        specified.
 */
RTMIDIAPI RtMidiInPtr rtmidi_in_create (enum RtMidiApi api, const char *clientName, unsigned int queueSizeLimit);

//! Deallocate the given pointer.
RTMIDIAPI void rtmidi_in_free (RtMidiInPtr device);

//! Returns the MIDI API specifier for the given instance of RtMidiIn.
RTMIDIAPI enum RtMidiApi rtmidi_in_get_current_api (RtMidiPtr device);

//! Set a callback function to be invoked for incoming MIDI messages.
RTMIDIAPI void rtmidi_in_set_callback (RtMidiInPtr device, RtMidiCCallback callback, void *userData);

//! Cancel use of the current callback function (if one exists).
RTMIDIAPI void rtmidi_in_cancel_callback (RtMidiInPtr device);

//! Specify whether certain MIDI message types should be queued or ignored during input.
RTMIDIAPI void rtmidi_in_ignore_types (RtMidiInPtr device, bool midiSysex, bool midiTime, bool midiSense);

/*! Fill the user-provided array with the data bytes for the next available
 * MIDI message in the input queue and return the event delta-time in seconds.
 *
 * \param message   Must point to a char* that is already allocated.
 *                  SYSEX messages maximum size being 1024, a statically
 *                  allocated array could
 *                  be sufficient. 
 * \param size      Is used to return the size of the message obtained. 
 */
RTMIDIAPI double rtmidi_in_get_message (RtMidiInPtr device, unsigned char **message, size_t * size);

/* RtMidiOut API */

//! Create a default RtMidiInPtr value, with no initialization.
RTMIDIAPI RtMidiOutPtr rtmidi_out_create_default ();

/*! Create a RtMidiOutPtr value, with given and clientName.
 *
 *  \param api            An optional API id can be specified.
 *  \param clientName     An optional client name can be specified. This
 *                        will be used to group the ports that are created
 *                        by the application.
 */
RTMIDIAPI RtMidiOutPtr rtmidi_out_create (enum RtMidiApi api, const char *clientName);

//! Deallocate the given pointer.
RTMIDIAPI void rtmidi_out_free (RtMidiOutPtr device);

//! Returns the MIDI API specifier for the given instance of RtMidiOut.
RTMIDIAPI enum RtMidiApi rtmidi_out_get_current_api (RtMidiPtr device);

//! Immediately send a single message out an open MIDI output port.
RTMIDIAPI int rtmidi_out_send_message (RtMidiOutPtr device, const unsigned char *message, int length);


#ifdef __cplusplus
}
#endif
#endif
