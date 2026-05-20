Guide: How to Apply and Verify Audio Filters in OBS Studio
==========================================================

This guide helps new users add audio filters to their microphones or audio sources and verify that the filters are actively changing the sound.

Step 1: Adding a Filter
-----------------------
1. Go to the **Audio Mixer** panel in the main OBS window.
2. Click the three dots (Options) next to your audio source (like *Mic/Aux*).
3. Select **Filters**.
4. Click the **+** icon at the bottom left of the screen to add a filter (such as a *Limiter* or *Gain*).

Step 2: Checking if the Filter is On
------------------------------------
* Look at the list of filters you added.
* Next to each filter name, there is an **Eye Icon**. 
* If the eye icon is bright, the filter is active. If there is a slash through the eye, the filter is turned off.

Step 3: Verifying the Sound Level (System Status)
-------------------------------------------------
Because the filter window does not have a live volume meter inside it, follow these steps to make sure your sound is changing:

1. Move the filter window to the side so you can see the main OBS window behind it.
2. Look at the green, yellow, and red volume bars in the **Audio Mixer** panel.
3. Speak into your microphone. 
4. Turn the filter off and on using the **Eye Icon** and watch the volume bars move. If the bars change size or behavior when you switch it on, your filter is working correctly!