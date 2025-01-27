using System;
using System.Windows.Forms;

using VGMToolbox.format;
using VGMToolbox.plugin;
using VGMToolbox.tools.stream;

namespace VGMToolbox.forms.stream
{
    public partial class MpegDemuxForm : AVgmtForm
    {
        public MpegDemuxForm(TreeNode pTreeNode)
            : base(pTreeNode)
        {
            // set title
            this.lblTitle.Text = "Video Demultiplexer";

            // hide the DoTask button since this is a drag and drop form
            this.btnDoTask.Hide();

            InitializeComponent();

            this.tbOutput.Text = "从游戏视频中分解流媒体." + Environment.NewLine;
            this.tbOutput.Text += "- 当前支持的格式: BIK, DSI (PS2), DVD Video, MO (Wii Only), MPEG1, MPEG2, PAM, PMF, PSS, SFD, THP, USM, XMV" + Environment.NewLine;
            this.tbOutput.Text += "- 如果MPEG不适用于您的文件,一定要试试DVD视频,因为它可以处理特殊的音频类型." + Environment.NewLine;
            this.tbOutput.Text += "- Bink音频文件并不总是使用RAD视频工具正确播放(binkplay.exe),但将使用RAD Video Tools“转换文件”正确转换为WAV(binkconv.exe)." + Environment.NewLine;
            this.tbOutput.Text += "- ASF/WMV解复用尚未重建到ASF流以进行输出." + Environment.NewLine;
            this.tbOutput.Text += "- MKVMerge可用于将原始.264数据添加到容器文件中以供播放." + Environment.NewLine;
            this.tbOutput.Text += "- 以下音频输出格式未知且不可测试：MO（A3 类型）." + Environment.NewLine;
            this.tbOutput.Text += "- 以下视频输出格式未知且不可测试：MO、XMV." + Environment.NewLine;


            this.initializeFormatList();
        }

        private void initializeFormatList()
        {
            this.comboFormat.Items.Clear();
            this.comboFormat.Items.Add("ASF (MS Advanced Systems Format)");
            this.comboFormat.Items.Add("BIK (Bink Video Container)");
            this.comboFormat.Items.Add("DSI (Racjin/Racdym PS2 Video)");
            this.comboFormat.Items.Add("DVD Video (VOB)");
            this.comboFormat.Items.Add("Electronic Arts VP6 (VP6)");
            this.comboFormat.Items.Add("Electronic Arts MPC (MPC)");
            //this.comboFormat.Items.Add("H4M (Hudson GameCube Video)");
            this.comboFormat.Items.Add("MO (Mobiclip)");
            this.comboFormat.Items.Add("MPEG");
            this.comboFormat.Items.Add("MPS (PSP UMD Movie)");
            this.comboFormat.Items.Add("PAM (PlayStation Advanced Movie)");
            this.comboFormat.Items.Add("PMF (PSP Movie Format)");
            this.comboFormat.Items.Add("PSS (PlayStation Stream)");
            this.comboFormat.Items.Add("SFD (CRI Sofdec Video)");
            this.comboFormat.Items.Add("THP");
            this.comboFormat.Items.Add("USM (CRI Movie 2)");
            this.comboFormat.Items.Add("WMV (MS Advanced Systems Format)");
            this.comboFormat.Items.Add("XMV (Xbox Media Video)");

            this.comboFormat.SelectedItem = "ASF (MS Advanced Systems Format)";
        }

        protected override IVgmtBackgroundWorker getBackgroundWorker()
        {
            return new MpegDemuxWorker();
        }
        protected override string getCancelMessage()
        {
            return "解复用流… 已取消";
        }
        protected override string getCompleteMessage()
        {
            return "解复用流… 完成";
        }
        protected override string getBeginMessage()
        {
            return "解复用流… 开始";
        }

        private void MpegDemuxForm_DragEnter(object sender, DragEventArgs e)
        {
            base.doDragEnter(sender, e);
        }

        private void MpegDemuxForm_DragDrop(object sender, DragEventArgs e)
        {
            MpegDemuxWorker.MpegDemuxStruct taskStruct = new MpegDemuxWorker.MpegDemuxStruct();

            // paths
            string[] s = (string[])e.Data.GetData(DataFormats.FileDrop, false);
            taskStruct.SourcePaths = s;

            // format
            taskStruct.SourceFormat = this.comboFormat.SelectedItem.ToString();

            // options
            taskStruct.AddHeader = this.cbAddHeader.Checked;
            taskStruct.SplitAudioTracks = this.cbSplitAudioTracks.Checked;
            taskStruct.ExtractAudio = (this.rbExtractAudioAndVideo.Checked ||
                                       this.rbExtractAudioOnly.Checked);
            taskStruct.ExtractVideo = (this.rbExtractAudioAndVideo.Checked ||
                                       this.rbExtractVideoOnly.Checked);

            base.backgroundWorker_Execute(taskStruct);
        }

        private void comboFormat_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (this.comboFormat.SelectedItem.ToString())
            {
                case "BIK (Bink Video Container)":
                    this.cbSplitAudioTracks.Enabled = true;
                    this.cbSplitAudioTracks.Checked = false;
                    this.cbAddHeader.Enabled = true;
                    this.cbAddHeader.Checked = true;
                    break;
                //case "ASF (MS Advanced Systems Format)":
                case "DSI (Racjin/Racdym PS2 Video)":
                case "H4M (Hudson GameCube Video)":
                case "MO (Mobiclip)":
                case "MPS (PSP UMD Movie)":
                case "PAM (PlayStation Advanced Movie)":
                case "PMF (PSP Movie Format)":
                //case "WMV (MS Advanced Systems Format)":
                case "XMV (Xbox Media Video)":
                    this.cbSplitAudioTracks.Enabled = false;
                    this.cbSplitAudioTracks.Checked = false;
                    this.cbAddHeader.Enabled = true;
                    this.cbAddHeader.Checked = true;
                    break;
                default:
                    this.cbSplitAudioTracks.Enabled = false;
                    this.cbSplitAudioTracks.Checked = false;
                    this.cbAddHeader.Checked = false;
                    this.cbAddHeader.Enabled = false;
                    break;
            }
        }
    }
}