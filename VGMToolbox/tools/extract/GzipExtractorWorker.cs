﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text;

using VGMToolbox.plugin;
using VGMToolbox.util;

namespace VGMToolbox.tools.extract
{
    class GzipExtractorWorker: AVgmtDragAndDropWorker, IVgmtBackgroundWorker
    {                
        public struct GzipExtractorStruct : IVgmtWorkerStruct
        {
            public bool DoDecompress;
            public long StartingOffset;
            
            private string[] sourcePaths;
            public string[] SourcePaths
            {
                get { return sourcePaths; }
                set { sourcePaths = value; }
            }
        }

        public GzipExtractorWorker() : base() { }

        protected override void DoTaskForFile(string pPath, IVgmtWorkerStruct pGzipExtractorStruct, DoWorkEventArgs e)
        {
            GzipExtractorStruct gzipExtractorStruct = (GzipExtractorStruct)pGzipExtractorStruct;

            this.progressStruct.Clear();

            if (gzipExtractorStruct.DoDecompress)
            {
                progressStruct.GenericMessage = String.Format("解压<{0}>{1}", pPath, Environment.NewLine);
            }
            else
            {
                progressStruct.GenericMessage = String.Format("压缩<{0}>{1}", pPath, Environment.NewLine);
            }
            ReportProgress(this.Progress, progressStruct);

            try
            {
                string outputFileName;

                if (gzipExtractorStruct.DoDecompress)
                {
                    outputFileName = Path.ChangeExtension(pPath, CompressionUtil.GzipDecompressOutputExtension);
                }
                else
                {
                    outputFileName = Path.ChangeExtension(pPath, CompressionUtil.GzipCompressOutputExtension);
                }
                
                using (FileStream fs = File.OpenRead(pPath))
                {
                    if (gzipExtractorStruct.StartingOffset > fs.Length)
                    {
                        throw new ArgumentOutOfRangeException("起始偏移量","偏移量不能大于文件大小.");
                    }

                    if (gzipExtractorStruct.DoDecompress)
                    {
                        CompressionUtil.DecompressGzipStreamToFile(fs, outputFileName, gzipExtractorStruct.StartingOffset);
                    }
                    else
                    {
                        CompressionUtil.CompressStreamToGzipFile(fs, outputFileName, gzipExtractorStruct.StartingOffset);
                    }
                }

                this.progressStruct.Clear();
                if (gzipExtractorStruct.DoDecompress)
                {
                    progressStruct.GenericMessage = String.Format("    {0}已解压.{1}", Path.GetFileName(outputFileName), Environment.NewLine);
                }
                else
                {
                    progressStruct.GenericMessage = String.Format("    {0}已压缩.{1}", Path.GetFileName(outputFileName), Environment.NewLine);
                }
                                
                ReportProgress(this.Progress, progressStruct);
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }
    }
}
