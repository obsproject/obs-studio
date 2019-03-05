/******************************************************************************* *\

Copyright (C) 2015-2016 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: mfxdispatcherprefixedfunctions.h

*******************************************************************************/


#ifndef __MFXDISPATCHERPREFIXEDFUNCTIONS_H__
#define __MFXDISPATCHERPREFIXEDFUNCTIONS_H__

// API 1.0 functions
#define MFXInit                          disp_MFXInit
#define MFXClose                         disp_MFXClose
#define MFXQueryIMPL                     disp_MFXQueryIMPL
#define MFXQueryVersion                  disp_MFXQueryVersion

#define MFXJoinSession                   disp_MFXJoinSession
#define MFXDisjoinSession                disp_MFXDisjoinSession
#define MFXCloneSession                  disp_MFXCloneSession
#define MFXSetPriority                   disp_MFXSetPriority
#define MFXGetPriority                   disp_MFXGetPriority

#define MFXVideoCORE_SetBufferAllocator  disp_MFXVideoCORE_SetBufferAllocator
#define MFXVideoCORE_SetFrameAllocator   disp_MFXVideoCORE_SetFrameAllocator
#define MFXVideoCORE_SetHandle           disp_MFXVideoCORE_SetHandle
#define MFXVideoCORE_GetHandle           disp_MFXVideoCORE_GetHandle
#define MFXVideoCORE_SyncOperation       disp_MFXVideoCORE_SyncOperation

#define MFXVideoENCODE_Query             disp_MFXVideoENCODE_Query
#define MFXVideoENCODE_QueryIOSurf       disp_MFXVideoENCODE_QueryIOSurf
#define MFXVideoENCODE_Init              disp_MFXVideoENCODE_Init
#define MFXVideoENCODE_Reset             disp_MFXVideoENCODE_Reset
#define MFXVideoENCODE_Close             disp_MFXVideoENCODE_Close
#define MFXVideoENCODE_GetVideoParam     disp_MFXVideoENCODE_GetVideoParam
#define MFXVideoENCODE_GetEncodeStat     disp_MFXVideoENCODE_GetEncodeStat
#define MFXVideoENCODE_EncodeFrameAsync  disp_MFXVideoENCODE_EncodeFrameAsync

#define MFXVideoDECODE_Query             disp_MFXVideoDECODE_Query
#define MFXVideoDECODE_DecodeHeader      disp_MFXVideoDECODE_DecodeHeader
#define MFXVideoDECODE_QueryIOSurf       disp_MFXVideoDECODE_QueryIOSurf
#define MFXVideoDECODE_Init              disp_MFXVideoDECODE_Init
#define MFXVideoDECODE_Reset             disp_MFXVideoDECODE_Reset
#define MFXVideoDECODE_Close             disp_MFXVideoDECODE_Close
#define MFXVideoDECODE_GetVideoParam     disp_MFXVideoDECODE_GetVideoParam
#define MFXVideoDECODE_GetDecodeStat     disp_MFXVideoDECODE_GetDecodeStat
#define MFXVideoDECODE_SetSkipMode       disp_MFXVideoDECODE_SetSkipMode
#define MFXVideoDECODE_GetPayload        disp_MFXVideoDECODE_GetPayload
#define MFXVideoDECODE_DecodeFrameAsync  disp_MFXVideoDECODE_DecodeFrameAsync

#define MFXVideoVPP_Query                disp_MFXVideoVPP_Query
#define MFXVideoVPP_QueryIOSurf          disp_MFXVideoVPP_QueryIOSurf
#define MFXVideoVPP_Init                 disp_MFXVideoVPP_Init
#define MFXVideoVPP_Reset                disp_MFXVideoVPP_Reset
#define MFXVideoVPP_Close                disp_MFXVideoVPP_Close

#define MFXVideoVPP_GetVideoParam        disp_MFXVideoVPP_GetVideoParam
#define MFXVideoVPP_GetVPPStat           disp_MFXVideoVPP_GetVPPStat
#define MFXVideoVPP_RunFrameVPPAsync     disp_MFXVideoVPP_RunFrameVPPAsync

// API 1.1 functions
#define MFXVideoUSER_Register            disp_MFXVideoUSER_Register
#define MFXVideoUSER_Unregister          disp_MFXVideoUSER_Unregister
#define MFXVideoUSER_ProcessFrameAsync   disp_MFXVideoUSER_ProcessFrameAsync

// API 1.10 functions

#define MFXVideoENC_Query                disp_MFXVideoENC_Query
#define MFXVideoENC_QueryIOSurf          disp_MFXVideoENC_QueryIOSurf
#define MFXVideoENC_Init                 disp_MFXVideoENC_Init
#define MFXVideoENC_Reset                disp_MFXVideoENC_Reset
#define MFXVideoENC_Close                disp_MFXVideoENC_Close
#define MFXVideoENC_ProcessFrameAsync    disp_MFXVideoENC_ProcessFrameAsync
#define MFXVideoVPP_RunFrameVPPAsyncEx   disp_MFXVideoVPP_RunFrameVPPAsyncEx
#define MFXVideoUSER_Load                disp_MFXVideoUSER_Load
#define MFXVideoUSER_UnLoad              disp_MFXVideoUSER_UnLoad

// API 1.11 functions

#define MFXVideoPAK_Query                disp_MFXVideoPAK_Query
#define MFXVideoPAK_QueryIOSurf          disp_MFXVideoPAK_QueryIOSurf
#define MFXVideoPAK_Init                 disp_MFXVideoPAK_Init
#define MFXVideoPAK_Reset                disp_MFXVideoPAK_Reset
#define MFXVideoPAK_Close                disp_MFXVideoPAK_Close
#define MFXVideoPAK_ProcessFrameAsync    disp_MFXVideoPAK_ProcessFrameAsync

// API 1.13 functions

#define MFXVideoUSER_LoadByPath          disp_MFXVideoUSER_LoadByPath

// API 1.14 functions
#define MFXInitEx                        disp_MFXInitEx
#define MFXDoWork                        disp_MFXDoWork

// Audio library functions

// API 1.8 functions

#define MFXAudioCORE_SyncOperation       disp_MFXAudioCORE_SyncOperation
#define MFXAudioENCODE_Query             disp_MFXAudioENCODE_Query
#define MFXAudioENCODE_QueryIOSize       disp_MFXAudioENCODE_QueryIOSize
#define MFXAudioENCODE_Init              disp_MFXAudioENCODE_Init
#define MFXAudioENCODE_Reset             disp_MFXAudioENCODE_Reset
#define MFXAudioENCODE_Close             disp_MFXAudioENCODE_Close
#define MFXAudioENCODE_GetAudioParam     disp_MFXAudioENCODE_GetAudioParam
#define MFXAudioENCODE_EncodeFrameAsync  disp_MFXAudioENCODE_EncodeFrameAsync

#define MFXAudioDECODE_Query             disp_MFXAudioDECODE_Query
#define MFXAudioDECODE_DecodeHeader      disp_MFXAudioDECODE_DecodeHeader
#define MFXAudioDECODE_Init              disp_MFXAudioDECODE_Init
#define MFXAudioDECODE_Reset             disp_MFXAudioDECODE_Reset
#define MFXAudioDECODE_Close             disp_MFXAudioDECODE_Close
#define MFXAudioDECODE_QueryIOSize       disp_MFXAudioDECODE_QueryIOSize
#define MFXAudioDECODE_GetAudioParam     disp_MFXAudioDECODE_GetAudioParam
#define MFXAudioDECODE_DecodeFrameAsync  disp_MFXAudioDECODE_DecodeFrameAsync

// API 1.9 functions

#define MFXAudioUSER_Register            disp_MFXAudioUSER_Register
#define MFXAudioUSER_Unregister          disp_MFXAudioUSER_Unregister
#define MFXAudioUSER_ProcessFrameAsync   disp_MFXAudioUSER_ProcessFrameAsync
#define MFXAudioUSER_Load                disp_MFXAudioUSER_Load
#define MFXAudioUSER_UnLoad              disp_MFXAudioUSER_UnLoad

// API 1.19 functions

#define MFXVideoENC_GetVideoParam        disp_MFXVideoENC_GetVideoParam
#define MFXVideoPAK_GetVideoParam        disp_MFXVideoPAK_GetVideoParam
#define MFXVideoCORE_QueryPlatform       disp_MFXVideoCORE_QueryPlatform
#define MFXVideoUSER_GetPlugin           disp_MFXVideoUSER_GetPlugin

#endif 