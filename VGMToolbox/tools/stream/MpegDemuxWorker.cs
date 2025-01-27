﻿using System;
using System.ComponentModel;
using System.IO;

using VGMToolbox.format;
using VGMToolbox.plugin;

namespace VGMToolbox.tools.stream
{
    class MpegDemuxWorker : AVgmtDragAndDropWorker, IVgmtBackgroundWorker
    {                
        public struct MpegDemuxStruct : IVgmtWorkerStruct
        {
            public string SourceFormat { set; get; }
            
            public bool ExtractAudio { set; get; }
            public bool ExtractVideo { set; get; }

            public bool AddHeader { set; get; }
            public bool SplitAudioTracks { set; get; }
            public bool AddPlaybackHacks { set; get; }

            public string[] SourcePaths { set; get; }
        }

        public MpegDemuxWorker() :
            base() { }

        protected override void DoTaskForFile(string path, IVgmtWorkerStruct pMpegDemuxStruct, DoWorkEventArgs e)
        {
            MpegDemuxStruct demuxStruct = (MpegDemuxStruct)pMpegDemuxStruct;
            MpegStream.DemuxOptionsStruct demuxOptions = new MpegStream.DemuxOptionsStruct();

            demuxOptions.ExtractAudio = demuxStruct.ExtractAudio;
            demuxOptions.ExtractVideo = demuxStruct.ExtractVideo;

            demuxOptions.AddHeader = demuxStruct.AddHeader;
            demuxOptions.SplitAudioStreams = demuxStruct.SplitAudioTracks;
            demuxOptions.AddPlaybackHacks = demuxStruct.AddPlaybackHacks;

            switch (demuxStruct.SourceFormat)
            {
                case "ASF (微软高级系统格式)":
                case "WMV (微软高级系统格式)":
                    MicrosoftAsfContainer asfStream = new MicrosoftAsfContainer(path);
                    asfStream.DemultiplexStreams(demuxOptions);
                    break;
                case "BIK (Bink视频容器)":
                    BinkStream binkStream = new BinkStream(path);
                    binkStream.DemultiplexStreams(demuxOptions);
                    break;
                case "DSI (Racjin/Racdym PS2视频)":
                    RacjinDsiStream dsiStream = new RacjinDsiStream(path);
                    dsiStream.DemultiplexStreams(demuxOptions);
                    break;
                case "DVD视频 (VOB)":
                    DvdVideoStream dvdStream = new DvdVideoStream(path);
                    dvdStream.DemultiplexStreams(demuxOptions);
                    break;
                case "On2 Technologies VP6开发(VP6)":
                    ElectronicArtsVp6Stream vp6Stream = new ElectronicArtsVp6Stream(path);
                    vp6Stream.DemultiplexStreams(demuxOptions);
                    break;
                case "电子艺界MPC (MPC)":
                    ElectronicArtsMpcStream mpcStream = new ElectronicArtsMpcStream(path);
                    mpcStream.DemultiplexStreams(demuxOptions);
                    break;
                case "H4M (Hudson GameCube视频)":
                    HudsonHvqm4VideoStream hvqmStream = new HudsonHvqm4VideoStream(path);
                    hvqmStream.DemultiplexStreams(demuxOptions);
                    break;
                case "MO (Mobiclip)":
                    MobiclipStream.MovieType movieType = MobiclipStream.GetMobiclipStreamType(path);                    

                    switch (movieType)
                    {
                        //case MobiclipStream.MovieType.NintendoDs:
                        //    MobiclipNdsStream mobiclipNdsStream = new MobiclipNdsStream(path);
                        //    mobiclipNdsStream.DemultiplexStreams(demuxOptions);
                        //    break;
                        case MobiclipStream.MovieType.Wii:
                            MobiclipWiiStream mobiclipWiiStream = new MobiclipWiiStream(path);
                            mobiclipWiiStream.DemultiplexStreams(demuxOptions);
                            break;
                        default:
                            throw new FormatException(String.Format("文件不支持的Mobiclip类型: {0}", Path.GetFileName(path)));
                    }

                    break;
                case "MPEG（galgame mpg，也可用ffmpeg）":
                    int mpegType = MpegStream.GetMpegStreamType(path);
                    
                    switch (mpegType)
                    {
                        case 1:
                            Mpeg1Stream mpeg1Stream = new Mpeg1Stream(path);
                            mpeg1Stream.DemultiplexStreams(demuxOptions);
                            break;
                        case 2:
                            Mpeg2Stream mpeg2Stream = new Mpeg2Stream(path);
                            mpeg2Stream.DemultiplexStreams(demuxOptions);
                            break;
                        default:
                            throw new FormatException(String.Format("不支持的MPEG类型，用于文件: {0}", Path.GetFileName(path)));
                    }
                    break;
                case "MPS (PSP UMD电影)":
                    SonyPspMpsStream mpsStream = new SonyPspMpsStream(path);
                    mpsStream.DemultiplexStreams(demuxOptions);
                    break;

                case "PAM (PlayStation高级电影)":
                    SonyPamStream pamStream = new SonyPamStream(path);
                    pamStream.DemultiplexStreams(demuxOptions);
                    break;

                case "PMF (PSP电影格式)":
                    SonyPmfStream pmfStream = new SonyPmfStream(path);
                    pmfStream.DemultiplexStreams(demuxOptions);
                    break;

                case "PSS (PlayStation流)":
                    SonyPssStream sps = new SonyPssStream(path);
                    sps.DemultiplexStreams(demuxOptions);
                    break;

                case "SFD (CRI Sofdec视频)":
                    SofdecStream ss = new SofdecStream(path);
                    ss.DemultiplexStreams(demuxOptions);
                    break;

                case "THP（任天堂wiiu）":
                    NintendoThpMovieStream thp = new NintendoThpMovieStream(path);
                    thp.DemultiplexStreams(demuxOptions);
                    break;
                case "USM (CRI第二代sofdec视频)":
                    CriUsmStream cus = new CriUsmStream(path);
                    cus.DemultiplexStreams(demuxOptions);
                    break;

                case "XMV (Xbox媒体视频)":
                    XmvStream xmv = new XmvStream(path);
                    xmv.DemultiplexStreams(demuxOptions);
                    break;

                default:
                    throw new FormatException("源格式未定义.");
            }

            this.outputBuffer.Append(Path.GetFileName(path) + Environment.NewLine);
        }               
    }
}
