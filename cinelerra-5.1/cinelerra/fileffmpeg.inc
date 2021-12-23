
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef FILEFFMPEG_INC
#define FILEFFMPEG_INC

class FileFFMPEG;
class FFMpegConfigNum;
class FFMpegAudioNum;
class FFMpegAudioBitrate;
class FFMpegAudioQuality;
class FFMpegVideoNum;
class FFMpegVideoBitrate;
class FFMpegVideoQuality;
class FFMpegPixFmtItems;
class FFMpegPixelFormat;
class FFMpegSampleFormat;
class FFMPEGConfigWindow;
class FFMPEGConfigAudio;
class FFAudioOptions;
class FFMPEGConfigAudioPopup;
class FFMPEGConfigVideo;
class FFVideoOptions;
class FFMPEGConfigVideoPopup;
class FFMPEGConfigFormat;
class FFFormatOptions;
class FFMPEGScanProgress;

class FFOptions_OptName;
class FFOptions_OptValue;
class FFOptions_Opt;
class FFOptions;
class FFOptions_OptPanel;
class FFOptionsKindItem;
class FFOptionsKind;
class FFOptionsUnits;
class FFOptionsText;
class FFOptionsApply;
class FFOptionsWindow;
class FFOptionsDialog;
class FFOptionsAudioDialog;
class FFOptionsVideoDialog;
class FFOptionsFormatDialog;
class FFOptionsViewAudio;
class FFOptionsViewVideo;
class FFOptionsViewFormat;
class FFOptionsFormatView;
class FFOptionsFormatViewDialog;

#define FF_KIND_CODEC 0
#define FF_KIND_FORMAT 1
#define FF_KIND_FFMPEG 2

#endif
