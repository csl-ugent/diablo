namespace EvaluateDiversity
{
    partial class MainAndClassifiers
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;
        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }
        #region Windows Form Designer generated code
        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainAndClassifiers));
            this.folderBrowserDialog1 = new System.Windows.Forms.FolderBrowserDialog();
            this.folderBrowserDialog2 = new System.Windows.Forms.FolderBrowserDialog();
            this.insThres = new System.Windows.Forms.TextBox();
            this.orderThres = new System.Windows.Forms.TextBox();
            this.execThres = new System.Windows.Forms.TextBox();
            this.cfgDistance = new System.Windows.Forms.TextBox();
            this.cfgThreshold = new System.Windows.Forms.TextBox();
            this.dfgDistance = new System.Windows.Forms.TextBox();
            this.dfgThreshold = new System.Windows.Forms.TextBox();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.maxMatches = new System.Windows.Forms.TextBox();
            this.panelParametersGlobalSettings = new System.Windows.Forms.Panel();
            this.TestWhichToConsider = new System.Windows.Forms.CheckBox();
            this.TestWhichToAdd = new System.Windows.Forms.CheckBox();
            this.label14 = new System.Windows.Forms.Label();
            this.label13 = new System.Windows.Forms.Label();
            this.label12 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.panel3 = new System.Windows.Forms.Panel();
            this.matchAllMatches = new System.Windows.Forms.RadioButton();
            this.matchBestOrUnmatchWorst = new System.Windows.Forms.RadioButton();
            this.panel6 = new System.Windows.Forms.Panel();
            this.TransitionLevel = new System.Windows.Forms.RadioButton();
            this.InstructionLevel = new System.Windows.Forms.RadioButton();
            this.BasicBlockLevel = new System.Windows.Forms.RadioButton();
            this.panel1 = new System.Windows.Forms.Panel();
            this.filter = new System.Windows.Forms.RadioButton();
            this.extend = new System.Windows.Forms.RadioButton();
            this.initial = new System.Windows.Forms.RadioButton();
            this.label18 = new System.Windows.Forms.Label();
            this.label17 = new System.Windows.Forms.Label();
            this.label16 = new System.Windows.Forms.Label();
            this.label15 = new System.Windows.Forms.Label();
            this.panel5 = new System.Windows.Forms.Panel();
            this.dfBoth = new System.Windows.Forms.RadioButton();
            this.dfDown = new System.Windows.Forms.RadioButton();
            this.dfUp = new System.Windows.Forms.RadioButton();
            this.panel4 = new System.Windows.Forms.Panel();
            this.textBox18 = new System.Windows.Forms.TextBox();
            this.cfBoth = new System.Windows.Forms.RadioButton();
            this.cfDown = new System.Windows.Forms.RadioButton();
            this.cfUp = new System.Windows.Forms.RadioButton();
            this.dataThres = new System.Windows.Forms.TextBox();
            this.syscallThres = new System.Windows.Forms.TextBox();
            this.DataCheckBox = new System.Windows.Forms.CheckBox();
            this.syscallCheckBox = new System.Windows.Forms.CheckBox();
            this.dataFlowCheckBox = new System.Windows.Forms.CheckBox();
            this.controlFlowCheckBox = new System.Windows.Forms.CheckBox();
            this.codeCheckBox = new System.Windows.Forms.CheckBox();
            this.orderCheckBox = new System.Windows.Forms.CheckBox();
            this.frequencyCheckBox = new System.Windows.Forms.CheckBox();
            this.AppliedBox = new System.Windows.Forms.GroupBox();
            this.appliedAllCheckbox = new System.Windows.Forms.CheckBox();
            this.SaveSettings = new System.Windows.Forms.Button();
            this.AppliedSettingsListBox = new System.Windows.Forms.CheckedListBox();
            this.toApplyCheckBoxAll = new System.Windows.Forms.CheckBox();
            this.toApplyListBox = new System.Windows.Forms.CheckedListBox();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.consoleToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.inspectionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.advancedToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.stepperToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.ToApplyBox = new System.Windows.Forms.GroupBox();
            this.RemoveSettings = new System.Windows.Forms.Button();
            this.LoadSettingsButton = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.button6 = new System.Windows.Forms.Button();
            this.button5 = new System.Windows.Forms.Button();
            this.button1 = new System.Windows.Forms.Button();
            this.button4 = new System.Windows.Forms.Button();
            this.button3 = new System.Windows.Forms.Button();
            this.panel7 = new System.Windows.Forms.Panel();
            this.dataVaryPareto = new System.Windows.Forms.CheckBox();
            this.frequencyVaryPareto = new System.Windows.Forms.CheckBox();
            this.syscallsVaryPareto = new System.Windows.Forms.CheckBox();
            this.orderVaryPareto = new System.Windows.Forms.CheckBox();
            this.dfVaryPareto = new System.Windows.Forms.CheckBox();
            this.codeVaryPareto = new System.Windows.Forms.CheckBox();
            this.cfVaryPareto = new System.Windows.Forms.CheckBox();
            this.panelParametersDirectionAndButton = new System.Windows.Forms.Panel();
            this.TestDirectionCF = new System.Windows.Forms.CheckBox();
            this.TestDirectionDF = new System.Windows.Forms.CheckBox();
            this.PanelLabelThreshold = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.panelparametersThreshold = new System.Windows.Forms.Panel();
            this.TestControlFlowThreshold = new System.Windows.Forms.CheckBox();
            this.TestDataFlowThreshold = new System.Windows.Forms.CheckBox();
            this.TestSyscallsThreshold = new System.Windows.Forms.CheckBox();
            this.stepIns = new System.Windows.Forms.TextBox();
            this.stepCF = new System.Windows.Forms.TextBox();
            this.stepOrder = new System.Windows.Forms.TextBox();
            this.stepFrequency = new System.Windows.Forms.TextBox();
            this.stepDF = new System.Windows.Forms.TextBox();
            this.stepData = new System.Windows.Forms.TextBox();
            this.stepSyscalls = new System.Windows.Forms.TextBox();
            this.toIns = new System.Windows.Forms.TextBox();
            this.toCF = new System.Windows.Forms.TextBox();
            this.toOrder = new System.Windows.Forms.TextBox();
            this.toFrequency = new System.Windows.Forms.TextBox();
            this.toDF = new System.Windows.Forms.TextBox();
            this.toData = new System.Windows.Forms.TextBox();
            this.toSyscalls = new System.Windows.Forms.TextBox();
            this.TestFrequencyThreshold = new System.Windows.Forms.CheckBox();
            this.TestOrderThreshold = new System.Windows.Forms.CheckBox();
            this.TestDataThreshold = new System.Windows.Forms.CheckBox();
            this.TestCodeThreshold = new System.Windows.Forms.CheckBox();
            this.panelLabelDistance = new System.Windows.Forms.Panel();
            this.label4 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.panelParametersDistance = new System.Windows.Forms.Panel();
            this.stepDistanceCF = new System.Windows.Forms.TextBox();
            this.toDistanceCF = new System.Windows.Forms.TextBox();
            this.toDistanceDF = new System.Windows.Forms.TextBox();
            this.TestDistanceCF = new System.Windows.Forms.CheckBox();
            this.TestDistanceDF = new System.Windows.Forms.CheckBox();
            this.stepDistanceDF = new System.Windows.Forms.TextBox();
            this.ApplySimple = new System.Windows.Forms.Button();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.tableLayoutPanel4 = new System.Windows.Forms.TableLayoutPanel();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.MoveToToApplySettings = new System.Windows.Forms.Button();
            this.LoadToApplySettings = new System.Windows.Forms.Button();
            this.LoadAppliedSettings = new System.Windows.Forms.Button();
            this.GuessMapping = new System.Windows.Forms.Button();
            this.copyToRight = new System.Windows.Forms.Button();
            this.ApplySettings = new System.Windows.Forms.Button();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.TableToApplyButtons = new System.Windows.Forms.TableLayoutPanel();
            this.TableAppliedButtons = new System.Windows.Forms.TableLayoutPanel();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.button2 = new System.Windows.Forms.Button();
            this.button7 = new System.Windows.Forms.Button();
            this.groupBox4.SuspendLayout();
            this.panelParametersGlobalSettings.SuspendLayout();
            this.panel3.SuspendLayout();
            this.panel6.SuspendLayout();
            this.panel1.SuspendLayout();
            this.panel5.SuspendLayout();
            this.panel4.SuspendLayout();
            this.AppliedBox.SuspendLayout();
            this.menuStrip1.SuspendLayout();
            this.ToApplyBox.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.panel7.SuspendLayout();
            this.panelParametersDirectionAndButton.SuspendLayout();
            this.PanelLabelThreshold.SuspendLayout();
            this.panelparametersThreshold.SuspendLayout();
            this.panelLabelDistance.SuspendLayout();
            this.panelParametersDistance.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.tableLayoutPanel4.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            this.TableToApplyButtons.SuspendLayout();
            this.TableAppliedButtons.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // folderBrowserDialog1
            // 
            this.folderBrowserDialog1.ShowNewFolderButton = false;
            // 
            // insThres
            // 
            this.insThres.Location = new System.Drawing.Point(139, 49);
            this.insThres.Name = "insThres";
            this.insThres.Size = new System.Drawing.Size(31, 20);
            this.insThres.TabIndex = 8;
            // 
            // orderThres
            // 
            this.orderThres.Location = new System.Drawing.Point(139, 165);
            this.orderThres.Name = "orderThres";
            this.orderThres.Size = new System.Drawing.Size(31, 20);
            this.orderThres.TabIndex = 13;
            // 
            // execThres
            // 
            this.execThres.Location = new System.Drawing.Point(139, 188);
            this.execThres.Name = "execThres";
            this.execThres.Size = new System.Drawing.Size(31, 20);
            this.execThres.TabIndex = 14;
            // 
            // cfgDistance
            // 
            this.cfgDistance.Location = new System.Drawing.Point(461, 97);
            this.cfgDistance.Name = "cfgDistance";
            this.cfgDistance.Size = new System.Drawing.Size(31, 20);
            this.cfgDistance.TabIndex = 42;
            // 
            // cfgThreshold
            // 
            this.cfgThreshold.Location = new System.Drawing.Point(139, 95);
            this.cfgThreshold.Name = "cfgThreshold";
            this.cfgThreshold.Size = new System.Drawing.Size(31, 20);
            this.cfgThreshold.TabIndex = 10;
            // 
            // dfgDistance
            // 
            this.dfgDistance.Location = new System.Drawing.Point(461, 124);
            this.dfgDistance.Name = "dfgDistance";
            this.dfgDistance.Size = new System.Drawing.Size(31, 20);
            this.dfgDistance.TabIndex = 42;
            // 
            // dfgThreshold
            // 
            this.dfgThreshold.Location = new System.Drawing.Point(139, 119);
            this.dfgThreshold.Name = "dfgThreshold";
            this.dfgThreshold.Size = new System.Drawing.Size(31, 20);
            this.dfgThreshold.TabIndex = 11;
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.maxMatches);
            this.groupBox4.Controls.Add(this.panelParametersGlobalSettings);
            this.groupBox4.Controls.Add(this.label14);
            this.groupBox4.Controls.Add(this.label13);
            this.groupBox4.Controls.Add(this.label12);
            this.groupBox4.Controls.Add(this.label10);
            this.groupBox4.Controls.Add(this.panel3);
            this.groupBox4.Controls.Add(this.panel6);
            this.groupBox4.Controls.Add(this.panel1);
            this.groupBox4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox4.Location = new System.Drawing.Point(3, 3);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(315, 219);
            this.groupBox4.TabIndex = 56;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "General Settings";
            // 
            // maxMatches
            // 
            this.maxMatches.Location = new System.Drawing.Point(8, 171);
            this.maxMatches.Name = "maxMatches";
            this.maxMatches.Size = new System.Drawing.Size(31, 20);
            this.maxMatches.TabIndex = 136;
            // 
            // panelParametersGlobalSettings
            // 
            this.panelParametersGlobalSettings.Controls.Add(this.TestWhichToConsider);
            this.panelParametersGlobalSettings.Controls.Add(this.TestWhichToAdd);
            this.panelParametersGlobalSettings.Location = new System.Drawing.Point(277, 119);
            this.panelParametersGlobalSettings.Name = "panelParametersGlobalSettings";
            this.panelParametersGlobalSettings.Size = new System.Drawing.Size(32, 79);
            this.panelParametersGlobalSettings.TabIndex = 79;
            this.panelParametersGlobalSettings.Visible = false;
            // 
            // TestWhichToConsider
            // 
            this.TestWhichToConsider.AutoSize = true;
            this.TestWhichToConsider.Enabled = false;
            this.TestWhichToConsider.Location = new System.Drawing.Point(9, 55);
            this.TestWhichToConsider.Name = "TestWhichToConsider";
            this.TestWhichToConsider.Size = new System.Drawing.Size(15, 14);
            this.TestWhichToConsider.TabIndex = 78;
            this.toolTip1.SetToolTip(this.TestWhichToConsider, "Unmapped and Mapped");
            this.TestWhichToConsider.UseVisualStyleBackColor = true;
            // 
            // TestWhichToAdd
            // 
            this.TestWhichToAdd.AutoSize = true;
            this.TestWhichToAdd.Location = new System.Drawing.Point(9, 6);
            this.TestWhichToAdd.Name = "TestWhichToAdd";
            this.TestWhichToAdd.Size = new System.Drawing.Size(15, 14);
            this.TestWhichToAdd.TabIndex = 77;
            this.toolTip1.SetToolTip(this.TestWhichToAdd, "Best and All");
            this.TestWhichToAdd.UseVisualStyleBackColor = true;
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label14.Location = new System.Drawing.Point(6, 16);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(40, 13);
            this.label14.TabIndex = 1;
            this.label14.Text = "Phase:";
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label13.Location = new System.Drawing.Point(6, 107);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(115, 13);
            this.label13.TabIndex = 2;
            this.label13.Text = "When to add mapping:";
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label12.Location = new System.Drawing.Point(6, 153);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(104, 13);
            this.label12.TabIndex = 3;
            this.label12.Text = "Maximum # matches";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label10.Location = new System.Drawing.Point(5, 63);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(60, 13);
            this.label10.TabIndex = 4;
            this.label10.Text = "Granularity:";
            // 
            // panel3
            // 
            this.panel3.Controls.Add(this.matchAllMatches);
            this.panel3.Controls.Add(this.matchBestOrUnmatchWorst);
            this.panel3.Location = new System.Drawing.Point(8, 123);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(259, 23);
            this.panel3.TabIndex = 60;
            // 
            // matchAllMatches
            // 
            this.matchAllMatches.AutoSize = true;
            this.matchAllMatches.Location = new System.Drawing.Point(110, 2);
            this.matchAllMatches.Name = "matchAllMatches";
            this.matchAllMatches.Size = new System.Drawing.Size(35, 17);
            this.matchAllMatches.TabIndex = 1;
            this.matchAllMatches.TabStop = true;
            this.matchAllMatches.Text = "all";
            this.matchAllMatches.UseVisualStyleBackColor = true;
            // 
            // matchBestOrUnmatchWorst
            // 
            this.matchBestOrUnmatchWorst.AutoSize = true;
            this.matchBestOrUnmatchWorst.Location = new System.Drawing.Point(4, 2);
            this.matchBestOrUnmatchWorst.Name = "matchBestOrUnmatchWorst";
            this.matchBestOrUnmatchWorst.Size = new System.Drawing.Size(45, 17);
            this.matchBestOrUnmatchWorst.TabIndex = 0;
            this.matchBestOrUnmatchWorst.TabStop = true;
            this.matchBestOrUnmatchWorst.Text = "best";
            this.matchBestOrUnmatchWorst.UseVisualStyleBackColor = true;
            // 
            // panel6
            // 
            this.panel6.Controls.Add(this.TransitionLevel);
            this.panel6.Controls.Add(this.InstructionLevel);
            this.panel6.Controls.Add(this.BasicBlockLevel);
            this.panel6.Location = new System.Drawing.Point(8, 79);
            this.panel6.Name = "panel6";
            this.panel6.Size = new System.Drawing.Size(293, 20);
            this.panel6.TabIndex = 60;
            // 
            // TransitionLevel
            // 
            this.TransitionLevel.AutoSize = true;
            this.TransitionLevel.Location = new System.Drawing.Point(110, 2);
            this.TransitionLevel.Name = "TransitionLevel";
            this.TransitionLevel.Size = new System.Drawing.Size(67, 17);
            this.TransitionLevel.TabIndex = 1;
            this.TransitionLevel.TabStop = true;
            this.TransitionLevel.Text = "transition";
            this.TransitionLevel.UseVisualStyleBackColor = true;
            // 
            // InstructionLevel
            // 
            this.InstructionLevel.AutoSize = true;
            this.InstructionLevel.Location = new System.Drawing.Point(212, 1);
            this.InstructionLevel.Name = "InstructionLevel";
            this.InstructionLevel.Size = new System.Drawing.Size(73, 17);
            this.InstructionLevel.TabIndex = 2;
            this.InstructionLevel.TabStop = true;
            this.InstructionLevel.Text = "instruction";
            this.InstructionLevel.UseVisualStyleBackColor = true;
            // 
            // BasicBlockLevel
            // 
            this.BasicBlockLevel.AutoSize = true;
            this.BasicBlockLevel.Location = new System.Drawing.Point(4, 1);
            this.BasicBlockLevel.Name = "BasicBlockLevel";
            this.BasicBlockLevel.Size = new System.Drawing.Size(79, 17);
            this.BasicBlockLevel.TabIndex = 0;
            this.BasicBlockLevel.TabStop = true;
            this.BasicBlockLevel.Text = "basic block";
            this.BasicBlockLevel.UseVisualStyleBackColor = true;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.filter);
            this.panel1.Controls.Add(this.extend);
            this.panel1.Controls.Add(this.initial);
            this.panel1.Location = new System.Drawing.Point(8, 32);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(259, 23);
            this.panel1.TabIndex = 59;
            // 
            // filter
            // 
            this.filter.AutoSize = true;
            this.filter.Location = new System.Drawing.Point(212, 0);
            this.filter.Name = "filter";
            this.filter.Size = new System.Drawing.Size(44, 17);
            this.filter.TabIndex = 0;
            this.filter.TabStop = true;
            this.filter.Text = "filter";
            this.filter.UseVisualStyleBackColor = true;
            this.filter.CheckedChanged += new System.EventHandler(this.filter_CheckedChanged);
            // 
            // extend
            // 
            this.extend.AutoSize = true;
            this.extend.Location = new System.Drawing.Point(110, 0);
            this.extend.Name = "extend";
            this.extend.Size = new System.Drawing.Size(57, 17);
            this.extend.TabIndex = 0;
            this.extend.TabStop = true;
            this.extend.Text = "extend";
            this.extend.UseVisualStyleBackColor = true;
            // 
            // initial
            // 
            this.initial.AutoSize = true;
            this.initial.Location = new System.Drawing.Point(4, 0);
            this.initial.Name = "initial";
            this.initial.Size = new System.Drawing.Size(48, 17);
            this.initial.TabIndex = 0;
            this.initial.TabStop = true;
            this.initial.Text = "initial";
            this.initial.UseVisualStyleBackColor = true;
            this.initial.CheckedChanged += new System.EventHandler(this.initial_CheckedChanged);
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label18.Location = new System.Drawing.Point(321, 23);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(49, 13);
            this.label18.TabIndex = 77;
            this.label18.Text = "Direction";
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label17.Location = new System.Drawing.Point(458, 23);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(49, 13);
            this.label17.TabIndex = 77;
            this.label17.Text = "Distance";
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label16.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label16.Location = new System.Drawing.Point(136, 22);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(54, 13);
            this.label16.TabIndex = 77;
            this.label16.Text = "Threshold";
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label15.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label15.Location = new System.Drawing.Point(6, 23);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(48, 13);
            this.label15.TabIndex = 0;
            this.label15.Text = "Classifier";
            // 
            // panel5
            // 
            this.panel5.Controls.Add(this.dfBoth);
            this.panel5.Controls.Add(this.dfDown);
            this.panel5.Controls.Add(this.dfUp);
            this.panel5.Location = new System.Drawing.Point(277, 119);
            this.panel5.Name = "panel5";
            this.panel5.Size = new System.Drawing.Size(138, 22);
            this.panel5.TabIndex = 72;
            // 
            // dfBoth
            // 
            this.dfBoth.AutoSize = true;
            this.dfBoth.Location = new System.Drawing.Point(88, 3);
            this.dfBoth.Name = "dfBoth";
            this.dfBoth.Size = new System.Drawing.Size(46, 17);
            this.dfBoth.TabIndex = 2;
            this.dfBoth.TabStop = true;
            this.dfBoth.Text = "both";
            this.dfBoth.UseVisualStyleBackColor = true;
            // 
            // dfDown
            // 
            this.dfDown.AutoSize = true;
            this.dfDown.Location = new System.Drawing.Point(40, 3);
            this.dfDown.Name = "dfDown";
            this.dfDown.Size = new System.Drawing.Size(51, 17);
            this.dfDown.TabIndex = 1;
            this.dfDown.TabStop = true;
            this.dfDown.Text = "down";
            this.dfDown.UseVisualStyleBackColor = true;
            // 
            // dfUp
            // 
            this.dfUp.AutoSize = true;
            this.dfUp.Location = new System.Drawing.Point(5, 3);
            this.dfUp.Name = "dfUp";
            this.dfUp.Size = new System.Drawing.Size(37, 17);
            this.dfUp.TabIndex = 0;
            this.dfUp.TabStop = true;
            this.dfUp.Text = "up";
            this.dfUp.UseVisualStyleBackColor = true;
            // 
            // panel4
            // 
            this.panel4.Controls.Add(this.textBox18);
            this.panel4.Controls.Add(this.cfBoth);
            this.panel4.Controls.Add(this.cfDown);
            this.panel4.Controls.Add(this.cfUp);
            this.panel4.Location = new System.Drawing.Point(277, 98);
            this.panel4.Name = "panel4";
            this.panel4.Size = new System.Drawing.Size(138, 22);
            this.panel4.TabIndex = 71;
            // 
            // textBox18
            // 
            this.textBox18.Location = new System.Drawing.Point(-6, 24);
            this.textBox18.Name = "textBox18";
            this.textBox18.Size = new System.Drawing.Size(31, 20);
            this.textBox18.TabIndex = 112;
            // 
            // cfBoth
            // 
            this.cfBoth.AutoSize = true;
            this.cfBoth.Location = new System.Drawing.Point(88, 3);
            this.cfBoth.Name = "cfBoth";
            this.cfBoth.Size = new System.Drawing.Size(46, 17);
            this.cfBoth.TabIndex = 2;
            this.cfBoth.TabStop = true;
            this.cfBoth.Text = "both";
            this.cfBoth.UseVisualStyleBackColor = true;
            // 
            // cfDown
            // 
            this.cfDown.AutoSize = true;
            this.cfDown.Location = new System.Drawing.Point(40, 3);
            this.cfDown.Name = "cfDown";
            this.cfDown.Size = new System.Drawing.Size(51, 17);
            this.cfDown.TabIndex = 1;
            this.cfDown.TabStop = true;
            this.cfDown.Text = "down";
            this.cfDown.UseVisualStyleBackColor = true;
            // 
            // cfUp
            // 
            this.cfUp.AutoSize = true;
            this.cfUp.Location = new System.Drawing.Point(5, 3);
            this.cfUp.Name = "cfUp";
            this.cfUp.Size = new System.Drawing.Size(37, 17);
            this.cfUp.TabIndex = 0;
            this.cfUp.TabStop = true;
            this.cfUp.Text = "up";
            this.cfUp.UseVisualStyleBackColor = true;
            // 
            // dataThres
            // 
            this.dataThres.Location = new System.Drawing.Point(139, 72);
            this.dataThres.Name = "dataThres";
            this.dataThres.Size = new System.Drawing.Size(31, 20);
            this.dataThres.TabIndex = 9;
            // 
            // syscallThres
            // 
            this.syscallThres.Location = new System.Drawing.Point(139, 142);
            this.syscallThres.Name = "syscallThres";
            this.syscallThres.Size = new System.Drawing.Size(31, 20);
            this.syscallThres.TabIndex = 12;
            // 
            // DataCheckBox
            // 
            this.DataCheckBox.AutoSize = true;
            this.DataCheckBox.Location = new System.Drawing.Point(9, 74);
            this.DataCheckBox.Name = "DataCheckBox";
            this.DataCheckBox.Size = new System.Drawing.Size(49, 17);
            this.DataCheckBox.TabIndex = 2;
            this.DataCheckBox.Text = "Data";
            this.DataCheckBox.ThreeState = true;
            this.DataCheckBox.UseVisualStyleBackColor = true;
            // 
            // syscallCheckBox
            // 
            this.syscallCheckBox.AutoSize = true;
            this.syscallCheckBox.Location = new System.Drawing.Point(9, 144);
            this.syscallCheckBox.Name = "syscallCheckBox";
            this.syscallCheckBox.Size = new System.Drawing.Size(64, 17);
            this.syscallCheckBox.TabIndex = 5;
            this.syscallCheckBox.Text = "Syscalls";
            this.syscallCheckBox.ThreeState = true;
            this.syscallCheckBox.UseVisualStyleBackColor = true;
            // 
            // dataFlowCheckBox
            // 
            this.dataFlowCheckBox.AutoSize = true;
            this.dataFlowCheckBox.Location = new System.Drawing.Point(9, 121);
            this.dataFlowCheckBox.Name = "dataFlowCheckBox";
            this.dataFlowCheckBox.Size = new System.Drawing.Size(40, 17);
            this.dataFlowCheckBox.TabIndex = 4;
            this.dataFlowCheckBox.Text = "DF";
            this.dataFlowCheckBox.ThreeState = true;
            this.dataFlowCheckBox.UseVisualStyleBackColor = true;
            // 
            // controlFlowCheckBox
            // 
            this.controlFlowCheckBox.AutoSize = true;
            this.controlFlowCheckBox.Location = new System.Drawing.Point(9, 98);
            this.controlFlowCheckBox.Name = "controlFlowCheckBox";
            this.controlFlowCheckBox.Size = new System.Drawing.Size(39, 17);
            this.controlFlowCheckBox.TabIndex = 3;
            this.controlFlowCheckBox.Text = "CF";
            this.controlFlowCheckBox.ThreeState = true;
            this.controlFlowCheckBox.UseVisualStyleBackColor = true;
            // 
            // codeCheckBox
            // 
            this.codeCheckBox.AutoSize = true;
            this.codeCheckBox.Location = new System.Drawing.Point(9, 51);
            this.codeCheckBox.Name = "codeCheckBox";
            this.codeCheckBox.Size = new System.Drawing.Size(51, 17);
            this.codeCheckBox.TabIndex = 1;
            this.codeCheckBox.Text = "Code";
            this.codeCheckBox.ThreeState = true;
            this.codeCheckBox.UseVisualStyleBackColor = true;
            // 
            // orderCheckBox
            // 
            this.orderCheckBox.AutoSize = true;
            this.orderCheckBox.Location = new System.Drawing.Point(9, 167);
            this.orderCheckBox.Name = "orderCheckBox";
            this.orderCheckBox.Size = new System.Drawing.Size(52, 17);
            this.orderCheckBox.TabIndex = 6;
            this.orderCheckBox.Text = "Order";
            this.orderCheckBox.ThreeState = true;
            this.orderCheckBox.UseVisualStyleBackColor = true;
            // 
            // frequencyCheckBox
            // 
            this.frequencyCheckBox.AutoSize = true;
            this.frequencyCheckBox.Location = new System.Drawing.Point(9, 190);
            this.frequencyCheckBox.Name = "frequencyCheckBox";
            this.frequencyCheckBox.Size = new System.Drawing.Size(76, 17);
            this.frequencyCheckBox.TabIndex = 7;
            this.frequencyCheckBox.Text = "Frequency";
            this.frequencyCheckBox.ThreeState = true;
            this.frequencyCheckBox.UseVisualStyleBackColor = true;
            // 
            // AppliedBox
            // 
            this.AppliedBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.AppliedBox.Controls.Add(this.appliedAllCheckbox);
            this.AppliedBox.Controls.Add(this.SaveSettings);
            this.AppliedBox.Controls.Add(this.AppliedSettingsListBox);
            this.AppliedBox.Location = new System.Drawing.Point(513, 298);
            this.AppliedBox.Name = "AppliedBox";
            this.AppliedBox.Size = new System.Drawing.Size(460, 336);
            this.AppliedBox.TabIndex = 60;
            this.AppliedBox.TabStop = false;
            this.AppliedBox.Text = "Applied";
            // 
            // appliedAllCheckbox
            // 
            this.appliedAllCheckbox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.appliedAllCheckbox.AutoSize = true;
            this.appliedAllCheckbox.Location = new System.Drawing.Point(423, 306);
            this.appliedAllCheckbox.Name = "appliedAllCheckbox";
            this.appliedAllCheckbox.Size = new System.Drawing.Size(15, 14);
            this.appliedAllCheckbox.TabIndex = 68;
            this.appliedAllCheckbox.UseVisualStyleBackColor = true;
            this.appliedAllCheckbox.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
            // 
            // SaveSettings
            // 
            this.SaveSettings.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.SaveSettings.Image = global::EvaluateDiversity.Properties.Resources.save_16;
            this.SaveSettings.Location = new System.Drawing.Point(420, 16);
            this.SaveSettings.Name = "SaveSettings";
            this.SaveSettings.Size = new System.Drawing.Size(34, 34);
            this.SaveSettings.TabIndex = 63;
            this.toolTip1.SetToolTip(this.SaveSettings, "Save All Settings to File");
            this.SaveSettings.UseVisualStyleBackColor = true;
            this.SaveSettings.Click += new System.EventHandler(this.SaveSettings_Click);
            // 
            // AppliedSettingsListBox
            // 
            this.AppliedSettingsListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.AppliedSettingsListBox.CheckOnClick = true;
            this.AppliedSettingsListBox.FormattingEnabled = true;
            this.AppliedSettingsListBox.HorizontalScrollbar = true;
            this.AppliedSettingsListBox.Location = new System.Drawing.Point(3, 16);
            this.AppliedSettingsListBox.Name = "AppliedSettingsListBox";
            this.AppliedSettingsListBox.Size = new System.Drawing.Size(414, 304);
            this.AppliedSettingsListBox.TabIndex = 0;
            // 
            // toApplyCheckBoxAll
            // 
            this.toApplyCheckBoxAll.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.toApplyCheckBoxAll.AutoSize = true;
            this.toApplyCheckBoxAll.Location = new System.Drawing.Point(422, 306);
            this.toApplyCheckBoxAll.Name = "toApplyCheckBoxAll";
            this.toApplyCheckBoxAll.Size = new System.Drawing.Size(15, 14);
            this.toApplyCheckBoxAll.TabIndex = 67;
            this.toApplyCheckBoxAll.UseVisualStyleBackColor = true;
            this.toApplyCheckBoxAll.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
            // 
            // toApplyListBox
            // 
            this.toApplyListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.toApplyListBox.CheckOnClick = true;
            this.toApplyListBox.FormattingEnabled = true;
            this.toApplyListBox.HorizontalScrollbar = true;
            this.toApplyListBox.Location = new System.Drawing.Point(3, 16);
            this.toApplyListBox.Name = "toApplyListBox";
            this.toApplyListBox.Size = new System.Drawing.Size(413, 304);
            this.toApplyListBox.TabIndex = 64;
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.FileName = "openFileDialog1";
            // 
            // saveFileDialog1
            // 
            this.saveFileDialog1.DefaultExt = "settings";
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.viewToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(976, 24);
            this.menuStrip1.TabIndex = 80;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openToolStripMenuItem,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "File";
            // 
            // openToolStripMenuItem
            // 
            this.openToolStripMenuItem.Name = "openToolStripMenuItem";
            this.openToolStripMenuItem.Size = new System.Drawing.Size(103, 22);
            this.openToolStripMenuItem.Text = "Open";
            this.openToolStripMenuItem.Click += new System.EventHandler(this.openToolStripMenuItem_Click);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(103, 22);
            this.exitToolStripMenuItem.Text = "Exit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // viewToolStripMenuItem
            // 
            this.viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.consoleToolStripMenuItem,
            this.inspectionToolStripMenuItem,
            this.advancedToolStripMenuItem,
            this.stepperToolStripMenuItem});
            this.viewToolStripMenuItem.Name = "viewToolStripMenuItem";
            this.viewToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.viewToolStripMenuItem.Text = "View";
            this.viewToolStripMenuItem.DropDownOpening += new System.EventHandler(this.viewToolStripMenuItem_Click);
            // 
            // consoleToolStripMenuItem
            // 
            this.consoleToolStripMenuItem.Name = "consoleToolStripMenuItem";
            this.consoleToolStripMenuItem.Size = new System.Drawing.Size(129, 22);
            this.consoleToolStripMenuItem.Text = "Console";
            this.consoleToolStripMenuItem.Click += new System.EventHandler(this.consoleToolStripMenuItem_Click);
            // 
            // inspectionToolStripMenuItem
            // 
            this.inspectionToolStripMenuItem.Name = "inspectionToolStripMenuItem";
            this.inspectionToolStripMenuItem.Size = new System.Drawing.Size(129, 22);
            this.inspectionToolStripMenuItem.Text = "Inspection";
            this.inspectionToolStripMenuItem.Click += new System.EventHandler(this.inspectionToolStripMenuItem_Click);
            // 
            // advancedToolStripMenuItem
            // 
            this.advancedToolStripMenuItem.Checked = true;
            this.advancedToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.advancedToolStripMenuItem.Name = "advancedToolStripMenuItem";
            this.advancedToolStripMenuItem.Size = new System.Drawing.Size(129, 22);
            this.advancedToolStripMenuItem.Text = "Advanced";
            this.advancedToolStripMenuItem.Click += new System.EventHandler(this.advancedToolStripMenuItem_Click);
            // 
            // stepperToolStripMenuItem
            // 
            this.stepperToolStripMenuItem.Name = "stepperToolStripMenuItem";
            this.stepperToolStripMenuItem.Size = new System.Drawing.Size(129, 22);
            this.stepperToolStripMenuItem.Text = "Stepper";
            this.stepperToolStripMenuItem.Click += new System.EventHandler(this.stepperToolStripMenuItem_Click);
            // 
            // ToApplyBox
            // 
            this.ToApplyBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.ToApplyBox.Controls.Add(this.RemoveSettings);
            this.ToApplyBox.Controls.Add(this.toApplyListBox);
            this.ToApplyBox.Controls.Add(this.toApplyCheckBoxAll);
            this.ToApplyBox.Controls.Add(this.LoadSettingsButton);
            this.ToApplyBox.Location = new System.Drawing.Point(3, 298);
            this.ToApplyBox.Name = "ToApplyBox";
            this.ToApplyBox.Size = new System.Drawing.Size(459, 336);
            this.ToApplyBox.TabIndex = 82;
            this.ToApplyBox.TabStop = false;
            this.ToApplyBox.Text = "ToApply";
            // 
            // RemoveSettings
            // 
            this.RemoveSettings.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.RemoveSettings.Image = global::EvaluateDiversity.Properties.Resources.delete_16x;
            this.RemoveSettings.Location = new System.Drawing.Point(419, 59);
            this.RemoveSettings.Name = "RemoveSettings";
            this.RemoveSettings.Size = new System.Drawing.Size(34, 34);
            this.RemoveSettings.TabIndex = 69;
            this.RemoveSettings.Text = "X";
            this.toolTip1.SetToolTip(this.RemoveSettings, "Delete Checked Settings");
            this.RemoveSettings.UseVisualStyleBackColor = true;
            this.RemoveSettings.Click += new System.EventHandler(this.RemoveSettingsFromListButton_Click);
            // 
            // LoadSettingsButton
            // 
            this.LoadSettingsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.LoadSettingsButton.Image = ((System.Drawing.Image)(resources.GetObject("LoadSettingsButton.Image")));
            this.LoadSettingsButton.Location = new System.Drawing.Point(419, 19);
            this.LoadSettingsButton.Name = "LoadSettingsButton";
            this.LoadSettingsButton.Size = new System.Drawing.Size(34, 34);
            this.LoadSettingsButton.TabIndex = 65;
            this.toolTip1.SetToolTip(this.LoadSettingsButton, "Load Settings from File");
            this.LoadSettingsButton.UseVisualStyleBackColor = true;
            this.LoadSettingsButton.Click += new System.EventHandler(this.LoadSettings_Click);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.button6);
            this.groupBox2.Controls.Add(this.button5);
            this.groupBox2.Controls.Add(this.button1);
            this.groupBox2.Controls.Add(this.button4);
            this.groupBox2.Controls.Add(this.button3);
            this.groupBox2.Controls.Add(this.panel7);
            this.groupBox2.Controls.Add(this.label16);
            this.groupBox2.Controls.Add(this.panelParametersDirectionAndButton);
            this.groupBox2.Controls.Add(this.PanelLabelThreshold);
            this.groupBox2.Controls.Add(this.panelparametersThreshold);
            this.groupBox2.Controls.Add(this.panelLabelDistance);
            this.groupBox2.Controls.Add(this.panelParametersDistance);
            this.groupBox2.Controls.Add(this.ApplySimple);
            this.groupBox2.Controls.Add(this.label18);
            this.groupBox2.Controls.Add(this.label15);
            this.groupBox2.Controls.Add(this.label17);
            this.groupBox2.Controls.Add(this.insThres);
            this.groupBox2.Controls.Add(this.cfgDistance);
            this.groupBox2.Controls.Add(this.cfgThreshold);
            this.groupBox2.Controls.Add(this.dfgDistance);
            this.groupBox2.Controls.Add(this.orderThres);
            this.groupBox2.Controls.Add(this.execThres);
            this.groupBox2.Controls.Add(this.dfgThreshold);
            this.groupBox2.Controls.Add(this.panel5);
            this.groupBox2.Controls.Add(this.frequencyCheckBox);
            this.groupBox2.Controls.Add(this.panel4);
            this.groupBox2.Controls.Add(this.orderCheckBox);
            this.groupBox2.Controls.Add(this.codeCheckBox);
            this.groupBox2.Controls.Add(this.controlFlowCheckBox);
            this.groupBox2.Controls.Add(this.dataThres);
            this.groupBox2.Controls.Add(this.dataFlowCheckBox);
            this.groupBox2.Controls.Add(this.syscallThres);
            this.groupBox2.Controls.Add(this.syscallCheckBox);
            this.groupBox2.Controls.Add(this.DataCheckBox);
            this.groupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox2.Location = new System.Drawing.Point(324, 3);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(637, 219);
            this.groupBox2.TabIndex = 83;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Classifier Specific Settings";
            // 
            // button6
            // 
            this.button6.Location = new System.Drawing.Point(282, 189);
            this.button6.Name = "button6";
            this.button6.Size = new System.Drawing.Size(75, 23);
            this.button6.TabIndex = 135;
            this.button6.Text = "experiment3";
            this.button6.UseVisualStyleBackColor = true;
            this.button6.Visible = false;
            this.button6.Click += new System.EventHandler(this.collectFalsePositiveRatesForSyntaxAndDataCombined);
            // 
            // button5
            // 
            this.button5.Location = new System.Drawing.Point(282, 166);
            this.button5.Name = "button5";
            this.button5.Size = new System.Drawing.Size(75, 23);
            this.button5.TabIndex = 134;
            this.button5.Text = "experiment2";
            this.button5.UseVisualStyleBackColor = true;
            this.button5.Visible = false;
            this.button5.Click += new System.EventHandler(this.collectSizesOfEquivalenceClasses);
            // 
            // button1
            // 
            this.button1.Image = global::EvaluateDiversity.Properties.Resources.redo_16;
            this.button1.Location = new System.Drawing.Point(493, 178);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(34, 34);
            this.button1.TabIndex = 88;
            this.toolTip1.SetToolTip(this.button1, "Try out combinations");
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Visible = false;
            this.button1.Click += new System.EventHandler(this.stepper);
            // 
            // button4
            // 
            this.button4.Location = new System.Drawing.Point(282, 143);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(75, 23);
            this.button4.TabIndex = 133;
            this.button4.Text = "experiment1";
            this.button4.UseVisualStyleBackColor = true;
            this.button4.Visible = false;
            this.button4.Click += new System.EventHandler(this.CollectFalsePositiveRates);
            // 
            // button3
            // 
            this.button3.Location = new System.Drawing.Point(365, 167);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(75, 23);
            this.button3.TabIndex = 132;
            this.button3.Text = "button3";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Visible = false;
            this.button3.Click += new System.EventHandler(this.instructionselectionvsinstructionsyntax);
            // 
            // panel7
            // 
            this.panel7.Controls.Add(this.dataVaryPareto);
            this.panel7.Controls.Add(this.frequencyVaryPareto);
            this.panel7.Controls.Add(this.syscallsVaryPareto);
            this.panel7.Controls.Add(this.orderVaryPareto);
            this.panel7.Controls.Add(this.dfVaryPareto);
            this.panel7.Controls.Add(this.codeVaryPareto);
            this.panel7.Controls.Add(this.cfVaryPareto);
            this.panel7.Location = new System.Drawing.Point(87, 47);
            this.panel7.Name = "panel7";
            this.panel7.Size = new System.Drawing.Size(24, 166);
            this.panel7.TabIndex = 130;
            this.panel7.Visible = false;
            // 
            // dataVaryPareto
            // 
            this.dataVaryPareto.AutoSize = true;
            this.dataVaryPareto.Location = new System.Drawing.Point(3, 28);
            this.dataVaryPareto.Name = "dataVaryPareto";
            this.dataVaryPareto.Size = new System.Drawing.Size(15, 14);
            this.dataVaryPareto.TabIndex = 124;
            this.dataVaryPareto.UseVisualStyleBackColor = true;
            // 
            // frequencyVaryPareto
            // 
            this.frequencyVaryPareto.AutoSize = true;
            this.frequencyVaryPareto.Location = new System.Drawing.Point(3, 144);
            this.frequencyVaryPareto.Name = "frequencyVaryPareto";
            this.frequencyVaryPareto.Size = new System.Drawing.Size(15, 14);
            this.frequencyVaryPareto.TabIndex = 129;
            this.frequencyVaryPareto.UseVisualStyleBackColor = true;
            // 
            // syscallsVaryPareto
            // 
            this.syscallsVaryPareto.AutoSize = true;
            this.syscallsVaryPareto.Location = new System.Drawing.Point(3, 98);
            this.syscallsVaryPareto.Name = "syscallsVaryPareto";
            this.syscallsVaryPareto.Size = new System.Drawing.Size(15, 14);
            this.syscallsVaryPareto.TabIndex = 127;
            this.syscallsVaryPareto.UseVisualStyleBackColor = true;
            // 
            // orderVaryPareto
            // 
            this.orderVaryPareto.AutoSize = true;
            this.orderVaryPareto.Location = new System.Drawing.Point(3, 121);
            this.orderVaryPareto.Name = "orderVaryPareto";
            this.orderVaryPareto.Size = new System.Drawing.Size(15, 14);
            this.orderVaryPareto.TabIndex = 128;
            this.orderVaryPareto.UseVisualStyleBackColor = true;
            // 
            // dfVaryPareto
            // 
            this.dfVaryPareto.AutoSize = true;
            this.dfVaryPareto.Location = new System.Drawing.Point(3, 75);
            this.dfVaryPareto.Name = "dfVaryPareto";
            this.dfVaryPareto.Size = new System.Drawing.Size(15, 14);
            this.dfVaryPareto.TabIndex = 126;
            this.dfVaryPareto.UseVisualStyleBackColor = true;
            // 
            // codeVaryPareto
            // 
            this.codeVaryPareto.AutoSize = true;
            this.codeVaryPareto.Location = new System.Drawing.Point(3, 5);
            this.codeVaryPareto.Name = "codeVaryPareto";
            this.codeVaryPareto.Size = new System.Drawing.Size(15, 14);
            this.codeVaryPareto.TabIndex = 123;
            this.codeVaryPareto.UseVisualStyleBackColor = true;
            // 
            // cfVaryPareto
            // 
            this.cfVaryPareto.AutoSize = true;
            this.cfVaryPareto.Location = new System.Drawing.Point(3, 52);
            this.cfVaryPareto.Name = "cfVaryPareto";
            this.cfVaryPareto.Size = new System.Drawing.Size(15, 14);
            this.cfVaryPareto.TabIndex = 125;
            this.cfVaryPareto.UseVisualStyleBackColor = true;
            // 
            // panelParametersDirectionAndButton
            // 
            this.panelParametersDirectionAndButton.Controls.Add(this.TestDirectionCF);
            this.panelParametersDirectionAndButton.Controls.Add(this.TestDirectionDF);
            this.panelParametersDirectionAndButton.Location = new System.Drawing.Point(421, 94);
            this.panelParametersDirectionAndButton.Name = "panelParametersDirectionAndButton";
            this.panelParametersDirectionAndButton.Size = new System.Drawing.Size(21, 47);
            this.panelParametersDirectionAndButton.TabIndex = 122;
            this.panelParametersDirectionAndButton.Visible = false;
            // 
            // TestDirectionCF
            // 
            this.TestDirectionCF.AutoSize = true;
            this.TestDirectionCF.Location = new System.Drawing.Point(0, 8);
            this.TestDirectionCF.Name = "TestDirectionCF";
            this.TestDirectionCF.Size = new System.Drawing.Size(15, 14);
            this.TestDirectionCF.TabIndex = 109;
            this.TestDirectionCF.UseVisualStyleBackColor = true;
            // 
            // TestDirectionDF
            // 
            this.TestDirectionDF.AutoSize = true;
            this.TestDirectionDF.Location = new System.Drawing.Point(0, 32);
            this.TestDirectionDF.Name = "TestDirectionDF";
            this.TestDirectionDF.Size = new System.Drawing.Size(15, 14);
            this.TestDirectionDF.TabIndex = 108;
            this.TestDirectionDF.UseVisualStyleBackColor = true;
            // 
            // PanelLabelThreshold
            // 
            this.PanelLabelThreshold.Controls.Add(this.label1);
            this.PanelLabelThreshold.Controls.Add(this.label2);
            this.PanelLabelThreshold.Controls.Add(this.label3);
            this.PanelLabelThreshold.Location = new System.Drawing.Point(139, 32);
            this.PanelLabelThreshold.Name = "PanelLabelThreshold";
            this.PanelLabelThreshold.Size = new System.Drawing.Size(114, 16);
            this.PanelLabelThreshold.TabIndex = 121;
            this.PanelLabelThreshold.Visible = false;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(77, 3);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(27, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "step";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(45, 3);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(16, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "to";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(3, 3);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(27, 13);
            this.label3.TabIndex = 0;
            this.label3.Text = "from";
            // 
            // panelparametersThreshold
            // 
            this.panelparametersThreshold.Controls.Add(this.TestControlFlowThreshold);
            this.panelparametersThreshold.Controls.Add(this.TestDataFlowThreshold);
            this.panelparametersThreshold.Controls.Add(this.TestSyscallsThreshold);
            this.panelparametersThreshold.Controls.Add(this.stepIns);
            this.panelparametersThreshold.Controls.Add(this.stepCF);
            this.panelparametersThreshold.Controls.Add(this.stepOrder);
            this.panelparametersThreshold.Controls.Add(this.stepFrequency);
            this.panelparametersThreshold.Controls.Add(this.stepDF);
            this.panelparametersThreshold.Controls.Add(this.stepData);
            this.panelparametersThreshold.Controls.Add(this.stepSyscalls);
            this.panelparametersThreshold.Controls.Add(this.toIns);
            this.panelparametersThreshold.Controls.Add(this.toCF);
            this.panelparametersThreshold.Controls.Add(this.toOrder);
            this.panelparametersThreshold.Controls.Add(this.toFrequency);
            this.panelparametersThreshold.Controls.Add(this.toDF);
            this.panelparametersThreshold.Controls.Add(this.toData);
            this.panelparametersThreshold.Controls.Add(this.toSyscalls);
            this.panelparametersThreshold.Controls.Add(this.TestFrequencyThreshold);
            this.panelparametersThreshold.Controls.Add(this.TestOrderThreshold);
            this.panelparametersThreshold.Controls.Add(this.TestDataThreshold);
            this.panelparametersThreshold.Controls.Add(this.TestCodeThreshold);
            this.panelparametersThreshold.Location = new System.Drawing.Point(176, 48);
            this.panelparametersThreshold.Name = "panelparametersThreshold";
            this.panelparametersThreshold.Size = new System.Drawing.Size(98, 163);
            this.panelparametersThreshold.TabIndex = 121;
            this.panelparametersThreshold.Visible = false;
            // 
            // TestControlFlowThreshold
            // 
            this.TestControlFlowThreshold.AutoSize = true;
            this.TestControlFlowThreshold.Location = new System.Drawing.Point(74, 53);
            this.TestControlFlowThreshold.Name = "TestControlFlowThreshold";
            this.TestControlFlowThreshold.Size = new System.Drawing.Size(15, 14);
            this.TestControlFlowThreshold.TabIndex = 109;
            this.toolTip1.SetToolTip(this.TestControlFlowThreshold, "Unmapped and Mapped");
            this.TestControlFlowThreshold.UseVisualStyleBackColor = true;
            // 
            // TestDataFlowThreshold
            // 
            this.TestDataFlowThreshold.AutoSize = true;
            this.TestDataFlowThreshold.Location = new System.Drawing.Point(74, 77);
            this.TestDataFlowThreshold.Name = "TestDataFlowThreshold";
            this.TestDataFlowThreshold.Size = new System.Drawing.Size(15, 14);
            this.TestDataFlowThreshold.TabIndex = 108;
            this.toolTip1.SetToolTip(this.TestDataFlowThreshold, "Unmapped and Mapped");
            this.TestDataFlowThreshold.UseVisualStyleBackColor = true;
            // 
            // TestSyscallsThreshold
            // 
            this.TestSyscallsThreshold.AutoSize = true;
            this.TestSyscallsThreshold.Location = new System.Drawing.Point(74, 100);
            this.TestSyscallsThreshold.Name = "TestSyscallsThreshold";
            this.TestSyscallsThreshold.Size = new System.Drawing.Size(15, 14);
            this.TestSyscallsThreshold.TabIndex = 107;
            this.toolTip1.SetToolTip(this.TestSyscallsThreshold, "Unmapped and Mapped");
            this.TestSyscallsThreshold.UseVisualStyleBackColor = true;
            // 
            // stepIns
            // 
            this.stepIns.Location = new System.Drawing.Point(37, 1);
            this.stepIns.Name = "stepIns";
            this.stepIns.Size = new System.Drawing.Size(31, 20);
            this.stepIns.TabIndex = 1;
            // 
            // stepCF
            // 
            this.stepCF.Location = new System.Drawing.Point(37, 47);
            this.stepCF.Name = "stepCF";
            this.stepCF.Size = new System.Drawing.Size(31, 20);
            this.stepCF.TabIndex = 5;
            // 
            // stepOrder
            // 
            this.stepOrder.Location = new System.Drawing.Point(37, 117);
            this.stepOrder.Name = "stepOrder";
            this.stepOrder.Size = new System.Drawing.Size(31, 20);
            this.stepOrder.TabIndex = 11;
            // 
            // stepFrequency
            // 
            this.stepFrequency.Location = new System.Drawing.Point(37, 140);
            this.stepFrequency.Name = "stepFrequency";
            this.stepFrequency.Size = new System.Drawing.Size(31, 20);
            this.stepFrequency.TabIndex = 13;
            // 
            // stepDF
            // 
            this.stepDF.Location = new System.Drawing.Point(37, 71);
            this.stepDF.Name = "stepDF";
            this.stepDF.Size = new System.Drawing.Size(31, 20);
            this.stepDF.TabIndex = 7;
            // 
            // stepData
            // 
            this.stepData.Location = new System.Drawing.Point(37, 24);
            this.stepData.Name = "stepData";
            this.stepData.Size = new System.Drawing.Size(31, 20);
            this.stepData.TabIndex = 3;
            // 
            // stepSyscalls
            // 
            this.stepSyscalls.Location = new System.Drawing.Point(37, 94);
            this.stepSyscalls.Name = "stepSyscalls";
            this.stepSyscalls.Size = new System.Drawing.Size(31, 20);
            this.stepSyscalls.TabIndex = 9;
            // 
            // toIns
            // 
            this.toIns.Location = new System.Drawing.Point(0, 1);
            this.toIns.Name = "toIns";
            this.toIns.Size = new System.Drawing.Size(31, 20);
            this.toIns.TabIndex = 0;
            // 
            // toCF
            // 
            this.toCF.Location = new System.Drawing.Point(0, 47);
            this.toCF.Name = "toCF";
            this.toCF.Size = new System.Drawing.Size(31, 20);
            this.toCF.TabIndex = 4;
            // 
            // toOrder
            // 
            this.toOrder.Location = new System.Drawing.Point(0, 117);
            this.toOrder.Name = "toOrder";
            this.toOrder.Size = new System.Drawing.Size(31, 20);
            this.toOrder.TabIndex = 10;
            // 
            // toFrequency
            // 
            this.toFrequency.Location = new System.Drawing.Point(0, 140);
            this.toFrequency.Name = "toFrequency";
            this.toFrequency.Size = new System.Drawing.Size(31, 20);
            this.toFrequency.TabIndex = 12;
            // 
            // toDF
            // 
            this.toDF.Location = new System.Drawing.Point(0, 71);
            this.toDF.Name = "toDF";
            this.toDF.Size = new System.Drawing.Size(31, 20);
            this.toDF.TabIndex = 6;
            // 
            // toData
            // 
            this.toData.Location = new System.Drawing.Point(0, 24);
            this.toData.Name = "toData";
            this.toData.Size = new System.Drawing.Size(31, 20);
            this.toData.TabIndex = 2;
            // 
            // toSyscalls
            // 
            this.toSyscalls.Location = new System.Drawing.Point(0, 94);
            this.toSyscalls.Name = "toSyscalls";
            this.toSyscalls.Size = new System.Drawing.Size(31, 20);
            this.toSyscalls.TabIndex = 8;
            // 
            // TestFrequencyThreshold
            // 
            this.TestFrequencyThreshold.AutoSize = true;
            this.TestFrequencyThreshold.Location = new System.Drawing.Point(74, 146);
            this.TestFrequencyThreshold.Name = "TestFrequencyThreshold";
            this.TestFrequencyThreshold.Size = new System.Drawing.Size(15, 14);
            this.TestFrequencyThreshold.TabIndex = 91;
            this.toolTip1.SetToolTip(this.TestFrequencyThreshold, "Unmapped and Mapped");
            this.TestFrequencyThreshold.UseVisualStyleBackColor = true;
            // 
            // TestOrderThreshold
            // 
            this.TestOrderThreshold.AutoSize = true;
            this.TestOrderThreshold.Location = new System.Drawing.Point(74, 123);
            this.TestOrderThreshold.Name = "TestOrderThreshold";
            this.TestOrderThreshold.Size = new System.Drawing.Size(15, 14);
            this.TestOrderThreshold.TabIndex = 90;
            this.toolTip1.SetToolTip(this.TestOrderThreshold, "Unmapped and Mapped");
            this.TestOrderThreshold.UseVisualStyleBackColor = true;
            // 
            // TestDataThreshold
            // 
            this.TestDataThreshold.AutoSize = true;
            this.TestDataThreshold.Location = new System.Drawing.Point(74, 30);
            this.TestDataThreshold.Name = "TestDataThreshold";
            this.TestDataThreshold.Size = new System.Drawing.Size(15, 14);
            this.TestDataThreshold.TabIndex = 89;
            this.toolTip1.SetToolTip(this.TestDataThreshold, "Unmapped and Mapped");
            this.TestDataThreshold.UseVisualStyleBackColor = true;
            // 
            // TestCodeThreshold
            // 
            this.TestCodeThreshold.AutoSize = true;
            this.TestCodeThreshold.Location = new System.Drawing.Point(74, 6);
            this.TestCodeThreshold.Name = "TestCodeThreshold";
            this.TestCodeThreshold.Size = new System.Drawing.Size(15, 14);
            this.TestCodeThreshold.TabIndex = 79;
            this.toolTip1.SetToolTip(this.TestCodeThreshold, "Unmapped and Mapped");
            this.TestCodeThreshold.UseVisualStyleBackColor = true;
            // 
            // panelLabelDistance
            // 
            this.panelLabelDistance.Controls.Add(this.label4);
            this.panelLabelDistance.Controls.Add(this.label5);
            this.panelLabelDistance.Controls.Add(this.label6);
            this.panelLabelDistance.Location = new System.Drawing.Point(461, 60);
            this.panelLabelDistance.Name = "panelLabelDistance";
            this.panelLabelDistance.Size = new System.Drawing.Size(114, 16);
            this.panelLabelDistance.TabIndex = 120;
            this.panelLabelDistance.Visible = false;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(77, 3);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(27, 13);
            this.label4.TabIndex = 117;
            this.label4.Text = "step";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(45, 3);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(16, 13);
            this.label5.TabIndex = 118;
            this.label5.Text = "to";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(3, 3);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(27, 13);
            this.label6.TabIndex = 116;
            this.label6.Text = "from";
            // 
            // panelParametersDistance
            // 
            this.panelParametersDistance.Controls.Add(this.stepDistanceCF);
            this.panelParametersDistance.Controls.Add(this.toDistanceCF);
            this.panelParametersDistance.Controls.Add(this.toDistanceDF);
            this.panelParametersDistance.Controls.Add(this.TestDistanceCF);
            this.panelParametersDistance.Controls.Add(this.TestDistanceDF);
            this.panelParametersDistance.Controls.Add(this.stepDistanceDF);
            this.panelParametersDistance.Location = new System.Drawing.Point(497, 97);
            this.panelParametersDistance.Name = "panelParametersDistance";
            this.panelParametersDistance.Size = new System.Drawing.Size(96, 51);
            this.panelParametersDistance.TabIndex = 119;
            this.panelParametersDistance.Visible = false;
            // 
            // stepDistanceCF
            // 
            this.stepDistanceCF.Location = new System.Drawing.Point(37, 2);
            this.stepDistanceCF.Name = "stepDistanceCF";
            this.stepDistanceCF.Size = new System.Drawing.Size(31, 20);
            this.stepDistanceCF.TabIndex = 113;
            // 
            // toDistanceCF
            // 
            this.toDistanceCF.Location = new System.Drawing.Point(0, 2);
            this.toDistanceCF.Name = "toDistanceCF";
            this.toDistanceCF.Size = new System.Drawing.Size(31, 20);
            this.toDistanceCF.TabIndex = 111;
            // 
            // toDistanceDF
            // 
            this.toDistanceDF.Location = new System.Drawing.Point(0, 26);
            this.toDistanceDF.Name = "toDistanceDF";
            this.toDistanceDF.Size = new System.Drawing.Size(31, 20);
            this.toDistanceDF.TabIndex = 110;
            // 
            // TestDistanceCF
            // 
            this.TestDistanceCF.AutoSize = true;
            this.TestDistanceCF.Location = new System.Drawing.Point(74, 5);
            this.TestDistanceCF.Name = "TestDistanceCF";
            this.TestDistanceCF.Size = new System.Drawing.Size(15, 14);
            this.TestDistanceCF.TabIndex = 109;
            this.TestDistanceCF.UseVisualStyleBackColor = true;
            // 
            // TestDistanceDF
            // 
            this.TestDistanceDF.AutoSize = true;
            this.TestDistanceDF.Location = new System.Drawing.Point(74, 32);
            this.TestDistanceDF.Name = "TestDistanceDF";
            this.TestDistanceDF.Size = new System.Drawing.Size(15, 14);
            this.TestDistanceDF.TabIndex = 108;
            this.TestDistanceDF.UseVisualStyleBackColor = true;
            // 
            // stepDistanceDF
            // 
            this.stepDistanceDF.Location = new System.Drawing.Point(37, 26);
            this.stepDistanceDF.Name = "stepDistanceDF";
            this.stepDistanceDF.Size = new System.Drawing.Size(31, 20);
            this.stepDistanceDF.TabIndex = 92;
            // 
            // ApplySimple
            // 
            this.ApplySimple.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.ApplySimple.Image = ((System.Drawing.Image)(resources.GetObject("ApplySimple.Image")));
            this.ApplySimple.Location = new System.Drawing.Point(537, 180);
            this.ApplySimple.Name = "ApplySimple";
            this.ApplySimple.Size = new System.Drawing.Size(34, 34);
            this.ApplySimple.TabIndex = 87;
            this.toolTip1.SetToolTip(this.ApplySimple, "Apply Checked Settings");
            this.ApplySimple.UseVisualStyleBackColor = true;
            this.ApplySimple.Visible = false;
            this.ApplySimple.Click += new System.EventHandler(this.GuessMapping_Click);
            // 
            // groupBox3
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.groupBox3, 3);
            this.groupBox3.Controls.Add(this.tableLayoutPanel4);
            this.groupBox3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox3.Location = new System.Drawing.Point(3, 3);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(970, 244);
            this.groupBox3.TabIndex = 84;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Settings";
            // 
            // tableLayoutPanel4
            // 
            this.tableLayoutPanel4.ColumnCount = 2;
            this.tableLayoutPanel4.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 33.36756F));
            this.tableLayoutPanel4.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 66.63245F));
            this.tableLayoutPanel4.Controls.Add(this.groupBox4, 0, 0);
            this.tableLayoutPanel4.Controls.Add(this.groupBox2, 1, 0);
            this.tableLayoutPanel4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel4.Location = new System.Drawing.Point(3, 16);
            this.tableLayoutPanel4.Name = "tableLayoutPanel4";
            this.tableLayoutPanel4.RowCount = 1;
            this.tableLayoutPanel4.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel4.Size = new System.Drawing.Size(964, 225);
            this.tableLayoutPanel4.TabIndex = 0;
            // 
            // MoveToToApplySettings
            // 
            this.MoveToToApplySettings.Image = ((System.Drawing.Image)(resources.GetObject("MoveToToApplySettings.Image")));
            this.MoveToToApplySettings.Location = new System.Drawing.Point(232, 3);
            this.MoveToToApplySettings.Name = "MoveToToApplySettings";
            this.MoveToToApplySettings.Size = new System.Drawing.Size(34, 33);
            this.MoveToToApplySettings.TabIndex = 79;
            this.toolTip1.SetToolTip(this.MoveToToApplySettings, "Copy Setting");
            this.MoveToToApplySettings.UseVisualStyleBackColor = true;
            this.MoveToToApplySettings.Click += new System.EventHandler(this.MoveSettingsToListButton_Click);
            // 
            // LoadToApplySettings
            // 
            this.LoadToApplySettings.Image = ((System.Drawing.Image)(resources.GetObject("LoadToApplySettings.Image")));
            this.LoadToApplySettings.Location = new System.Drawing.Point(187, 3);
            this.LoadToApplySettings.Name = "LoadToApplySettings";
            this.LoadToApplySettings.Size = new System.Drawing.Size(34, 33);
            this.LoadToApplySettings.TabIndex = 78;
            this.toolTip1.SetToolTip(this.LoadToApplySettings, "Inspect Selected Setting");
            this.LoadToApplySettings.UseVisualStyleBackColor = true;
            this.LoadToApplySettings.Click += new System.EventHandler(this.loadSettingsButton_Click);
            // 
            // LoadAppliedSettings
            // 
            this.LoadAppliedSettings.Image = ((System.Drawing.Image)(resources.GetObject("LoadAppliedSettings.Image")));
            this.LoadAppliedSettings.Location = new System.Drawing.Point(188, 3);
            this.LoadAppliedSettings.Name = "LoadAppliedSettings";
            this.LoadAppliedSettings.Size = new System.Drawing.Size(34, 33);
            this.LoadAppliedSettings.TabIndex = 81;
            this.toolTip1.SetToolTip(this.LoadAppliedSettings, "Inspect Selected Setting");
            this.LoadAppliedSettings.UseVisualStyleBackColor = true;
            this.LoadAppliedSettings.Click += new System.EventHandler(this.LoadAppliedSettings_Click);
            // 
            // GuessMapping
            // 
            this.GuessMapping.Image = ((System.Drawing.Image)(resources.GetObject("GuessMapping.Image")));
            this.GuessMapping.Location = new System.Drawing.Point(233, 3);
            this.GuessMapping.Name = "GuessMapping";
            this.GuessMapping.Size = new System.Drawing.Size(34, 33);
            this.GuessMapping.TabIndex = 30;
            this.toolTip1.SetToolTip(this.GuessMapping, "Apply Setting");
            this.GuessMapping.UseVisualStyleBackColor = true;
            this.GuessMapping.Click += new System.EventHandler(this.GuessMapping_Click);
            // 
            // copyToRight
            // 
            this.copyToRight.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.copyToRight.Image = ((System.Drawing.Image)(resources.GetObject("copyToRight.Image")));
            this.copyToRight.Location = new System.Drawing.Point(3, 176);
            this.copyToRight.Name = "copyToRight";
            this.copyToRight.Size = new System.Drawing.Size(33, 34);
            this.copyToRight.TabIndex = 1;
            this.toolTip1.SetToolTip(this.copyToRight, "Copy Checked Settings");
            this.copyToRight.UseVisualStyleBackColor = true;
            this.copyToRight.Click += new System.EventHandler(this.copyToRight_Click);
            // 
            // ApplySettings
            // 
            this.ApplySettings.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.ApplySettings.Image = ((System.Drawing.Image)(resources.GetObject("ApplySettings.Image")));
            this.ApplySettings.Location = new System.Drawing.Point(3, 131);
            this.ApplySettings.Name = "ApplySettings";
            this.ApplySettings.Size = new System.Drawing.Size(33, 34);
            this.ApplySettings.TabIndex = 0;
            this.toolTip1.SetToolTip(this.ApplySettings, "Apply Checked Settings");
            this.ApplySettings.UseVisualStyleBackColor = true;
            this.ApplySettings.Click += new System.EventHandler(this.ApplySettings_Click);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 3;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 45F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Controls.Add(this.statusStrip1, 0, 3);
            this.tableLayoutPanel1.Controls.Add(this.AppliedBox, 2, 2);
            this.tableLayoutPanel1.Controls.Add(this.TableToApplyButtons, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.TableAppliedButtons, 2, 1);
            this.tableLayoutPanel1.Controls.Add(this.ToApplyBox, 0, 2);
            this.tableLayoutPanel1.Controls.Add(this.tableLayoutPanel2, 1, 2);
            this.tableLayoutPanel1.Controls.Add(this.groupBox3, 0, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 24);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 4;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 250F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 45F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 25F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(976, 662);
            this.tableLayoutPanel1.TabIndex = 85;
            // 
            // statusStrip1
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.statusStrip1, 3);
            this.statusStrip1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1});
            this.statusStrip1.Location = new System.Drawing.Point(0, 637);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(976, 25);
            this.statusStrip1.TabIndex = 86;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(36, 20);
            this.toolStripStatusLabel1.Text = "Score";
            // 
            // TableToApplyButtons
            // 
            this.TableToApplyButtons.ColumnCount = 4;
            this.TableToApplyButtons.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.TableToApplyButtons.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 45F));
            this.TableToApplyButtons.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 45F));
            this.TableToApplyButtons.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.TableToApplyButtons.Controls.Add(this.MoveToToApplySettings, 2, 0);
            this.TableToApplyButtons.Controls.Add(this.LoadToApplySettings, 1, 0);
            this.TableToApplyButtons.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TableToApplyButtons.Location = new System.Drawing.Point(3, 253);
            this.TableToApplyButtons.Name = "TableToApplyButtons";
            this.TableToApplyButtons.RowCount = 1;
            this.TableToApplyButtons.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.TableToApplyButtons.Size = new System.Drawing.Size(459, 39);
            this.TableToApplyButtons.TabIndex = 83;
            // 
            // TableAppliedButtons
            // 
            this.TableAppliedButtons.ColumnCount = 4;
            this.TableAppliedButtons.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.TableAppliedButtons.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 45F));
            this.TableAppliedButtons.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 45F));
            this.TableAppliedButtons.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.TableAppliedButtons.Controls.Add(this.LoadAppliedSettings, 1, 0);
            this.TableAppliedButtons.Controls.Add(this.GuessMapping, 2, 0);
            this.TableAppliedButtons.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TableAppliedButtons.Location = new System.Drawing.Point(513, 253);
            this.TableAppliedButtons.Name = "TableAppliedButtons";
            this.TableAppliedButtons.RowCount = 1;
            this.TableAppliedButtons.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.TableAppliedButtons.Size = new System.Drawing.Size(460, 39);
            this.TableAppliedButtons.TabIndex = 84;
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.ColumnCount = 1;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.Controls.Add(this.copyToRight, 0, 2);
            this.tableLayoutPanel2.Controls.Add(this.ApplySettings, 0, 1);
            this.tableLayoutPanel2.Controls.Add(this.button2, 0, 0);
            this.tableLayoutPanel2.Controls.Add(this.button7, 0, 3);
            this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel2.Location = new System.Drawing.Point(468, 298);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 4;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 45F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 45F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(39, 336);
            this.tableLayoutPanel2.TabIndex = 87;
            // 
            // button2
            // 
            this.button2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.button2.Image = ((System.Drawing.Image)(resources.GetObject("button2.Image")));
            this.button2.Location = new System.Drawing.Point(3, 86);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(33, 34);
            this.button2.TabIndex = 0;
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.ApplySettingsAndReport_Click);
            // 
            // button7
            // 
            this.button7.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.button7.Image = ((System.Drawing.Image)(resources.GetObject("button7.Image")));
            this.button7.Location = new System.Drawing.Point(3, 299);
            this.button7.Name = "button7";
            this.button7.Size = new System.Drawing.Size(33, 34);
            this.button7.TabIndex = 2;
            this.button7.UseVisualStyleBackColor = true;
            this.button7.Click += new System.EventHandler(this.button7_Click);
            // 
            // MainAndClassifiers
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(976, 686);
            this.Controls.Add(this.tableLayoutPanel1);
            this.Controls.Add(this.menuStrip1);
            this.DoubleBuffered = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "MainAndClassifiers";
            this.Text = "Evaluating Diversity";
            this.groupBox4.ResumeLayout(false);
            this.groupBox4.PerformLayout();
            this.panelParametersGlobalSettings.ResumeLayout(false);
            this.panelParametersGlobalSettings.PerformLayout();
            this.panel3.ResumeLayout(false);
            this.panel3.PerformLayout();
            this.panel6.ResumeLayout(false);
            this.panel6.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.panel5.ResumeLayout(false);
            this.panel5.PerformLayout();
            this.panel4.ResumeLayout(false);
            this.panel4.PerformLayout();
            this.AppliedBox.ResumeLayout(false);
            this.AppliedBox.PerformLayout();
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.ToApplyBox.ResumeLayout(false);
            this.ToApplyBox.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.panel7.ResumeLayout(false);
            this.panel7.PerformLayout();
            this.panelParametersDirectionAndButton.ResumeLayout(false);
            this.panelParametersDirectionAndButton.PerformLayout();
            this.PanelLabelThreshold.ResumeLayout(false);
            this.PanelLabelThreshold.PerformLayout();
            this.panelparametersThreshold.ResumeLayout(false);
            this.panelparametersThreshold.PerformLayout();
            this.panelLabelDistance.ResumeLayout(false);
            this.panelLabelDistance.PerformLayout();
            this.panelParametersDistance.ResumeLayout(false);
            this.panelParametersDistance.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.tableLayoutPanel4.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.TableToApplyButtons.ResumeLayout(false);
            this.TableAppliedButtons.ResumeLayout(false);
            this.tableLayoutPanel2.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }
        #endregion

        private System.Windows.Forms.Button GuessMapping;
        private System.Windows.Forms.FolderBrowserDialog folderBrowserDialog1;
        private System.Windows.Forms.FolderBrowserDialog folderBrowserDialog2;
        private System.Windows.Forms.TextBox insThres;
        private System.Windows.Forms.TextBox orderThres;
        private System.Windows.Forms.TextBox execThres;
        private System.Windows.Forms.TextBox cfgDistance;
        private System.Windows.Forms.TextBox cfgThreshold;
        private System.Windows.Forms.TextBox dfgDistance;
        private System.Windows.Forms.TextBox dfgThreshold;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.RadioButton filter;
        private System.Windows.Forms.RadioButton extend;
        private System.Windows.Forms.RadioButton initial;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.RadioButton matchAllMatches;
        private System.Windows.Forms.RadioButton matchBestOrUnmatchWorst;
        private System.Windows.Forms.CheckBox orderCheckBox;
        private System.Windows.Forms.CheckBox frequencyCheckBox;
        private System.Windows.Forms.CheckBox dataFlowCheckBox;
        private System.Windows.Forms.CheckBox controlFlowCheckBox;
        private System.Windows.Forms.CheckBox codeCheckBox;
        private System.Windows.Forms.TextBox dataThres;
        private System.Windows.Forms.TextBox syscallThres;
        private System.Windows.Forms.CheckBox DataCheckBox;
        private System.Windows.Forms.CheckBox syscallCheckBox;
        private System.Windows.Forms.Panel panel5;
        private System.Windows.Forms.RadioButton dfBoth;
        private System.Windows.Forms.RadioButton dfDown;
        private System.Windows.Forms.RadioButton dfUp;
        private System.Windows.Forms.Panel panel4;
        private System.Windows.Forms.RadioButton cfBoth;
        private System.Windows.Forms.RadioButton cfDown;
        private System.Windows.Forms.RadioButton cfUp;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Panel panel6;
        private System.Windows.Forms.RadioButton BasicBlockLevel;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.Label label15;
        private System.Windows.Forms.Label label18;
        private System.Windows.Forms.Label label17;
        private System.Windows.Forms.Label label16;
        private System.Windows.Forms.GroupBox AppliedBox;
        private System.Windows.Forms.Button ApplySettings;
        private System.Windows.Forms.CheckedListBox AppliedSettingsListBox;
        private System.Windows.Forms.Button copyToRight;
        private System.Windows.Forms.Button LoadSettingsButton;
        private System.Windows.Forms.CheckedListBox toApplyListBox;
        private System.Windows.Forms.Button SaveSettings;
        private System.Windows.Forms.CheckBox appliedAllCheckbox;
        private System.Windows.Forms.CheckBox toApplyCheckBoxAll;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.SaveFileDialog saveFileDialog1;
        private System.Windows.Forms.Button RemoveSettings;
        private System.Windows.Forms.Button MoveToToApplySettings;
        private System.Windows.Forms.Button LoadToApplySettings;
        private System.Windows.Forms.RadioButton InstructionLevel;
        private System.Windows.Forms.RadioButton TransitionLevel;
        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem viewToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem consoleToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem inspectionToolStripMenuItem;
        private System.Windows.Forms.Button LoadAppliedSettings;
        private System.Windows.Forms.GroupBox ToApplyBox;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel4;
        private System.Windows.Forms.TableLayoutPanel TableToApplyButtons;
        private System.Windows.Forms.TableLayoutPanel TableAppliedButtons;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.Button ApplySimple;
        private System.Windows.Forms.ToolStripMenuItem advancedToolStripMenuItem;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.CheckBox TestWhichToConsider;
        private System.Windows.Forms.CheckBox TestWhichToAdd;
        private System.Windows.Forms.TextBox stepDistanceDF;
        private System.Windows.Forms.TextBox stepDistanceCF;
        private System.Windows.Forms.TextBox textBox18;
        private System.Windows.Forms.TextBox toDistanceCF;
        private System.Windows.Forms.TextBox toDistanceDF;
        private System.Windows.Forms.CheckBox TestDirectionCF;
        private System.Windows.Forms.CheckBox TestDirectionDF;
        private System.Windows.Forms.CheckBox TestDistanceCF;
        private System.Windows.Forms.CheckBox TestDistanceDF;
        private System.Windows.Forms.Panel panelLabelDistance;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Panel panelParametersDistance;
        private System.Windows.Forms.CheckBox TestControlFlowThreshold;
        private System.Windows.Forms.CheckBox TestDataFlowThreshold;
        private System.Windows.Forms.CheckBox TestSyscallsThreshold;
        private System.Windows.Forms.TextBox stepIns;
        private System.Windows.Forms.TextBox stepCF;
        private System.Windows.Forms.TextBox stepOrder;
        private System.Windows.Forms.TextBox stepFrequency;
        private System.Windows.Forms.TextBox stepDF;
        private System.Windows.Forms.TextBox stepData;
        private System.Windows.Forms.TextBox stepSyscalls;
        private System.Windows.Forms.TextBox toIns;
        private System.Windows.Forms.TextBox toCF;
        private System.Windows.Forms.TextBox toOrder;
        private System.Windows.Forms.TextBox toFrequency;
        private System.Windows.Forms.TextBox toDF;
        private System.Windows.Forms.TextBox toData;
        private System.Windows.Forms.TextBox toSyscalls;
        private System.Windows.Forms.CheckBox TestFrequencyThreshold;
        private System.Windows.Forms.CheckBox TestOrderThreshold;
        private System.Windows.Forms.CheckBox TestDataThreshold;
        private System.Windows.Forms.CheckBox TestCodeThreshold;
        private System.Windows.Forms.Panel panelParametersGlobalSettings;
        private System.Windows.Forms.Panel PanelLabelThreshold;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Panel panelparametersThreshold;
        private System.Windows.Forms.ToolStripMenuItem stepperToolStripMenuItem;
        private System.Windows.Forms.Panel panelParametersDirectionAndButton;
        private System.Windows.Forms.CheckBox frequencyVaryPareto;
        private System.Windows.Forms.CheckBox orderVaryPareto;
        private System.Windows.Forms.CheckBox codeVaryPareto;
        private System.Windows.Forms.CheckBox cfVaryPareto;
        private System.Windows.Forms.CheckBox dfVaryPareto;
        private System.Windows.Forms.CheckBox syscallsVaryPareto;
        private System.Windows.Forms.CheckBox dataVaryPareto;
        private System.Windows.Forms.Panel panel7;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.Button button4;
        private System.Windows.Forms.Button button5;
        private System.Windows.Forms.Button button6;
        private System.Windows.Forms.TextBox maxMatches;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.Button button7;
    }
}
