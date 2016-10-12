namespace EvaluateDiversity
{
    partial class Form1
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.Program1 = new System.Windows.Forms.ListBox();
            this.consoleOutput = new System.Windows.Forms.TextBox();
            this.Program2 = new System.Windows.Forms.ListBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.falseNegativesCombined = new System.Windows.Forms.ListBox();
            this.falsePositives = new System.Windows.Forms.ListBox();
            this.falseNegatives = new System.Windows.Forms.ListBox();
            this.Ok = new System.Windows.Forms.ListBox();
            this.GuessMapping = new System.Windows.Forms.Button();
            this.scoreGOAL = new System.Windows.Forms.Label();
            this.folderBrowserDialog1 = new System.Windows.Forms.FolderBrowserDialog();
            this.folderBrowserDialog2 = new System.Windows.Forms.FolderBrowserDialog();
            this.loadProgram1 = new System.Windows.Forms.Button();
            this.program1TextBox = new System.Windows.Forms.TextBox();
            this.program2TextBox = new System.Windows.Forms.TextBox();
            this.loadProgram2 = new System.Windows.Forms.Button();
            this.load = new System.Windows.Forms.Button();
            this.correct = new System.Windows.Forms.ListBox();
            this.label5 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.swap = new System.Windows.Forms.Button();
            this.scoreOK = new System.Windows.Forms.Label();
            this.insThres = new System.Windows.Forms.TextBox();
            this.orderThres = new System.Windows.Forms.TextBox();
            this.execThres = new System.Windows.Forms.TextBox();
            this.falsePositivesCombined = new System.Windows.Forms.ListBox();
            this.label11 = new System.Windows.Forms.Label();
            this.cfgLevel = new System.Windows.Forms.TextBox();
            this.cfgThreshold = new System.Windows.Forms.TextBox();
            this.dfgLevel = new System.Windows.Forms.TextBox();
            this.dfgThreshold = new System.Windows.Forms.TextBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.label18 = new System.Windows.Forms.Label();
            this.label17 = new System.Windows.Forms.Label();
            this.label16 = new System.Windows.Forms.Label();
            this.label15 = new System.Windows.Forms.Label();
            this.label14 = new System.Windows.Forms.Label();
            this.label13 = new System.Windows.Forms.Label();
            this.label12 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.panel5 = new System.Windows.Forms.Panel();
            this.dfBoth = new System.Windows.Forms.RadioButton();
            this.dfDown = new System.Windows.Forms.RadioButton();
            this.dfUp = new System.Windows.Forms.RadioButton();
            this.panel4 = new System.Windows.Forms.Panel();
            this.cfBoth = new System.Windows.Forms.RadioButton();
            this.cfDown = new System.Windows.Forms.RadioButton();
            this.cfUp = new System.Windows.Forms.RadioButton();
            this.label9 = new System.Windows.Forms.Label();
            this.minSizeBbl = new System.Windows.Forms.TextBox();
            this.dataThres = new System.Windows.Forms.TextBox();
            this.syscallThres = new System.Windows.Forms.TextBox();
            this.DataCheckBox = new System.Windows.Forms.CheckBox();
            this.syscallCheckBox = new System.Windows.Forms.CheckBox();
            this.dataFlowCheckBox = new System.Windows.Forms.CheckBox();
            this.controlFlowCheckBox = new System.Windows.Forms.CheckBox();
            this.codeCheckBox = new System.Windows.Forms.CheckBox();
            this.orderCheckBox = new System.Windows.Forms.CheckBox();
            this.frequencyCheckBox = new System.Windows.Forms.CheckBox();
            this.panel3 = new System.Windows.Forms.Panel();
            this.matchAllMatches = new System.Windows.Forms.RadioButton();
            this.matchBestOrUnmatchWorst = new System.Windows.Forms.RadioButton();
            this.matchFirst = new System.Windows.Forms.RadioButton();
            this.panel6 = new System.Windows.Forms.Panel();
            this.TransitionLevel = new System.Windows.Forms.RadioButton();
            this.InstructionLevel = new System.Windows.Forms.RadioButton();
            this.BasicBlockLevel = new System.Windows.Forms.RadioButton();
            this.panel2 = new System.Windows.Forms.Panel();
            this.tryAll = new System.Windows.Forms.RadioButton();
            this.tryOnlyUnmapped = new System.Windows.Forms.RadioButton();
            this.panel1 = new System.Windows.Forms.Panel();
            this.filter = new System.Windows.Forms.RadioButton();
            this.extend = new System.Windows.Forms.RadioButton();
            this.initial = new System.Windows.Forms.RadioButton();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.scoreFP = new System.Windows.Forms.Label();
            this.scoreFN = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.groupBox6 = new System.Windows.Forms.GroupBox();
            this.RemoveSettings = new System.Windows.Forms.Button();
            this.appliedAllCheckbox = new System.Windows.Forms.CheckBox();
            this.toApplyCheckBoxAll = new System.Windows.Forms.CheckBox();
            this.label20 = new System.Windows.Forms.Label();
            this.LoadSettingsButton = new System.Windows.Forms.Button();
            this.toApplyListBox = new System.Windows.Forms.CheckedListBox();
            this.SaveSettings = new System.Windows.Forms.Button();
            this.copyToRight = new System.Windows.Forms.Button();
            this.label19 = new System.Windows.Forms.Label();
            this.ApplySettings = new System.Windows.Forms.Button();
            this.AppliedSettingsListBox = new System.Windows.Forms.CheckedListBox();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
            this.button2 = new System.Windows.Forms.Button();
            this.button3 = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.panel5.SuspendLayout();
            this.panel4.SuspendLayout();
            this.panel3.SuspendLayout();
            this.panel6.SuspendLayout();
            this.panel2.SuspendLayout();
            this.panel1.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.groupBox6.SuspendLayout();
            this.SuspendLayout();
            // 
            // Program1
            // 
            this.Program1.DrawMode = System.Windows.Forms.DrawMode.OwnerDrawFixed;
            this.Program1.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Program1.FormattingEnabled = true;
            this.Program1.ItemHeight = 14;
            this.Program1.Location = new System.Drawing.Point(12, 22);
            this.Program1.Name = "Program1";
            this.Program1.Size = new System.Drawing.Size(278, 270);
            this.Program1.TabIndex = 0;
            this.Program1.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.lstBox_DrawItem);
            this.Program1.SelectedIndexChanged += new System.EventHandler(this.Program1_SelectedIndexChanged);
            // 
            // consoleOutput
            // 
            this.consoleOutput.Location = new System.Drawing.Point(202, 710);
            this.consoleOutput.Multiline = true;
            this.consoleOutput.Name = "consoleOutput";
            this.consoleOutput.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.consoleOutput.Size = new System.Drawing.Size(681, 134);
            this.consoleOutput.TabIndex = 3;
            this.consoleOutput.TextChanged += new System.EventHandler(this.consoleOutput_TextChanged);
            // 
            // Program2
            // 
            this.Program2.DrawMode = System.Windows.Forms.DrawMode.OwnerDrawFixed;
            this.Program2.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Program2.FormattingEnabled = true;
            this.Program2.ItemHeight = 14;
            this.Program2.Location = new System.Drawing.Point(296, 22);
            this.Program2.Name = "Program2";
            this.Program2.Size = new System.Drawing.Size(278, 270);
            this.Program2.TabIndex = 5;
            this.Program2.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.rightLstBox_DrawItem);
            this.Program2.SelectedIndexChanged += new System.EventHandler(this.Program2_SelectedIndexChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(7, 18);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(77, 13);
            this.label2.TabIndex = 7;
            this.label2.Text = "False Positives";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(6, 80);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(83, 13);
            this.label3.TabIndex = 10;
            this.label3.Text = "False Negatives";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(6, 142);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(22, 13);
            this.label4.TabIndex = 13;
            this.label4.Text = "OK";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 17);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(83, 13);
            this.label1.TabIndex = 17;
            this.label1.Text = "False Negatives";
            // 
            // falseNegativesCombined
            // 
            this.falseNegativesCombined.FormattingEnabled = true;
            this.falseNegativesCombined.Location = new System.Drawing.Point(6, 33);
            this.falseNegativesCombined.Name = "falseNegativesCombined";
            this.falseNegativesCombined.Size = new System.Drawing.Size(266, 43);
            this.falseNegativesCombined.TabIndex = 20;
            this.falseNegativesCombined.SelectedIndexChanged += new System.EventHandler(this.faulty_SelectedIndexChanged);
            // 
            // falsePositives
            // 
            this.falsePositives.FormattingEnabled = true;
            this.falsePositives.Location = new System.Drawing.Point(6, 34);
            this.falsePositives.Name = "falsePositives";
            this.falsePositives.Size = new System.Drawing.Size(266, 43);
            this.falsePositives.TabIndex = 21;
            this.falsePositives.SelectedIndexChanged += new System.EventHandler(this.falsePositives_SelectedIndexChanged);
            // 
            // falseNegatives
            // 
            this.falseNegatives.FormattingEnabled = true;
            this.falseNegatives.Location = new System.Drawing.Point(6, 96);
            this.falseNegatives.Name = "falseNegatives";
            this.falseNegatives.Size = new System.Drawing.Size(266, 43);
            this.falseNegatives.TabIndex = 22;
            this.falseNegatives.SelectedIndexChanged += new System.EventHandler(this.falseNegatives_SelectedIndexChanged);
            // 
            // Ok
            // 
            this.Ok.FormattingEnabled = true;
            this.Ok.Location = new System.Drawing.Point(6, 158);
            this.Ok.Name = "Ok";
            this.Ok.Size = new System.Drawing.Size(266, 43);
            this.Ok.TabIndex = 23;
            this.Ok.SelectedIndexChanged += new System.EventHandler(this.Ok_SelectedIndexChanged);
            // 
            // GuessMapping
            // 
            this.GuessMapping.Location = new System.Drawing.Point(113, 383);
            this.GuessMapping.Name = "GuessMapping";
            this.GuessMapping.Size = new System.Drawing.Size(80, 23);
            this.GuessMapping.TabIndex = 30;
            this.GuessMapping.Text = "Guess";
            this.GuessMapping.UseVisualStyleBackColor = true;
            this.GuessMapping.Click += new System.EventHandler(this.GuessMapping_Click);
            // 
            // scoreGOAL
            // 
            this.scoreGOAL.AutoSize = true;
            this.scoreGOAL.Location = new System.Drawing.Point(6, 55);
            this.scoreGOAL.Name = "scoreGOAL";
            this.scoreGOAL.Size = new System.Drawing.Size(32, 13);
            this.scoreGOAL.TabIndex = 31;
            this.scoreGOAL.Text = "Goal:";
            // 
            // folderBrowserDialog1
            // 
            this.folderBrowserDialog1.RootFolder = System.Environment.SpecialFolder.Personal;
            this.folderBrowserDialog1.ShowNewFolderButton = false;
            // 
            // loadProgram1
            // 
            this.loadProgram1.Location = new System.Drawing.Point(6, 19);
            this.loadProgram1.Name = "loadProgram1";
            this.loadProgram1.Size = new System.Drawing.Size(75, 23);
            this.loadProgram1.TabIndex = 32;
            this.loadProgram1.Text = "Program 1";
            this.loadProgram1.UseVisualStyleBackColor = true;
            this.loadProgram1.Click += new System.EventHandler(this.loadProgram1_Click);
            // 
            // program1TextBox
            // 
            this.program1TextBox.Location = new System.Drawing.Point(87, 20);
            this.program1TextBox.Name = "program1TextBox";
            this.program1TextBox.Size = new System.Drawing.Size(211, 20);
            this.program1TextBox.TabIndex = 33;
            this.program1TextBox.Text = "C:\\Documents and Settings\\banckaer\\My Documents\\evaluatingDiversity\\mcf\\options2\\" +
                "version1\\";
            // 
            // program2TextBox
            // 
            this.program2TextBox.Location = new System.Drawing.Point(87, 46);
            this.program2TextBox.Name = "program2TextBox";
            this.program2TextBox.Size = new System.Drawing.Size(211, 20);
            this.program2TextBox.TabIndex = 35;
            this.program2TextBox.Text = "C:\\Documents and Settings\\banckaer\\My Documents\\evaluatingDiversity\\mcf\\options2\\" +
                "version2\\";
            // 
            // loadProgram2
            // 
            this.loadProgram2.Location = new System.Drawing.Point(6, 45);
            this.loadProgram2.Name = "loadProgram2";
            this.loadProgram2.Size = new System.Drawing.Size(75, 23);
            this.loadProgram2.TabIndex = 34;
            this.loadProgram2.Text = "Program 2";
            this.loadProgram2.UseVisualStyleBackColor = true;
            this.loadProgram2.Click += new System.EventHandler(this.loadProgram2_Click);
            // 
            // load
            // 
            this.load.Location = new System.Drawing.Point(87, 74);
            this.load.Name = "load";
            this.load.Size = new System.Drawing.Size(75, 23);
            this.load.TabIndex = 36;
            this.load.Text = "Load";
            this.load.UseVisualStyleBackColor = true;
            this.load.Click += new System.EventHandler(this.load_Click);
            // 
            // correct
            // 
            this.correct.FormattingEnabled = true;
            this.correct.Location = new System.Drawing.Point(6, 158);
            this.correct.Name = "correct";
            this.correct.Size = new System.Drawing.Size(266, 43);
            this.correct.TabIndex = 37;
            this.correct.SelectedIndexChanged += new System.EventHandler(this.correct_SelectedIndexChanged);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(3, 142);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(22, 13);
            this.label5.TabIndex = 38;
            this.label5.Text = "OK";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label6.Location = new System.Drawing.Point(200, 694);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(45, 13);
            this.label6.TabIndex = 40;
            this.label6.Text = "Console";
            // 
            // swap
            // 
            this.swap.Location = new System.Drawing.Point(255, 296);
            this.swap.Name = "swap";
            this.swap.Size = new System.Drawing.Size(77, 23);
            this.swap.TabIndex = 19;
            this.swap.Text = "Swap";
            this.swap.UseVisualStyleBackColor = true;
            this.swap.Click += new System.EventHandler(this.swap_Click);
            // 
            // scoreOK
            // 
            this.scoreOK.AutoSize = true;
            this.scoreOK.Location = new System.Drawing.Point(6, 16);
            this.scoreOK.Name = "scoreOK";
            this.scoreOK.Size = new System.Drawing.Size(32, 13);
            this.scoreOK.TabIndex = 41;
            this.scoreOK.Text = "#OK:";
            // 
            // insThres
            // 
            this.insThres.Location = new System.Drawing.Point(86, 216);
            this.insThres.Name = "insThres";
            this.insThres.Size = new System.Drawing.Size(31, 20);
            this.insThres.TabIndex = 42;
            // 
            // orderThres
            // 
            this.orderThres.Location = new System.Drawing.Point(86, 332);
            this.orderThres.Name = "orderThres";
            this.orderThres.Size = new System.Drawing.Size(31, 20);
            this.orderThres.TabIndex = 43;
            // 
            // execThres
            // 
            this.execThres.Location = new System.Drawing.Point(86, 355);
            this.execThres.Name = "execThres";
            this.execThres.Size = new System.Drawing.Size(31, 20);
            this.execThres.TabIndex = 44;
            // 
            // falsePositivesCombined
            // 
            this.falsePositivesCombined.FormattingEnabled = true;
            this.falsePositivesCombined.Location = new System.Drawing.Point(6, 96);
            this.falsePositivesCombined.Name = "falsePositivesCombined";
            this.falsePositivesCombined.Size = new System.Drawing.Size(266, 43);
            this.falsePositivesCombined.TabIndex = 49;
            this.falsePositivesCombined.SelectedIndexChanged += new System.EventHandler(this.falsePositivesCombined_SelectedIndexChanged);
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(3, 80);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(77, 13);
            this.label11.TabIndex = 48;
            this.label11.Text = "False Positives";
            // 
            // cfgLevel
            // 
            this.cfgLevel.Location = new System.Drawing.Point(130, 262);
            this.cfgLevel.Name = "cfgLevel";
            this.cfgLevel.Size = new System.Drawing.Size(31, 20);
            this.cfgLevel.TabIndex = 42;
            // 
            // cfgThreshold
            // 
            this.cfgThreshold.Location = new System.Drawing.Point(86, 262);
            this.cfgThreshold.Name = "cfgThreshold";
            this.cfgThreshold.Size = new System.Drawing.Size(31, 20);
            this.cfgThreshold.TabIndex = 42;
            // 
            // dfgLevel
            // 
            this.dfgLevel.Location = new System.Drawing.Point(130, 286);
            this.dfgLevel.Name = "dfgLevel";
            this.dfgLevel.Size = new System.Drawing.Size(31, 20);
            this.dfgLevel.TabIndex = 42;
            // 
            // dfgThreshold
            // 
            this.dfgThreshold.Location = new System.Drawing.Point(86, 286);
            this.dfgThreshold.Name = "dfgThreshold";
            this.dfgThreshold.Size = new System.Drawing.Size(31, 20);
            this.dfgThreshold.TabIndex = 50;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.Ok);
            this.groupBox1.Controls.Add(this.falseNegatives);
            this.groupBox1.Controls.Add(this.falsePositives);
            this.groupBox1.Controls.Add(this.label4);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Location = new System.Drawing.Point(12, 320);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(278, 210);
            this.groupBox1.TabIndex = 53;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Selected Instruction";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.falsePositivesCombined);
            this.groupBox2.Controls.Add(this.label11);
            this.groupBox2.Controls.Add(this.label5);
            this.groupBox2.Controls.Add(this.correct);
            this.groupBox2.Controls.Add(this.falseNegativesCombined);
            this.groupBox2.Controls.Add(this.label1);
            this.groupBox2.Location = new System.Drawing.Point(296, 320);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(278, 210);
            this.groupBox2.TabIndex = 54;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Entire Program";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.load);
            this.groupBox3.Controls.Add(this.program2TextBox);
            this.groupBox3.Controls.Add(this.loadProgram2);
            this.groupBox3.Controls.Add(this.program1TextBox);
            this.groupBox3.Controls.Add(this.loadProgram1);
            this.groupBox3.Location = new System.Drawing.Point(579, 6);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(304, 103);
            this.groupBox3.TabIndex = 55;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Load";
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.label18);
            this.groupBox4.Controls.Add(this.label17);
            this.groupBox4.Controls.Add(this.label16);
            this.groupBox4.Controls.Add(this.label15);
            this.groupBox4.Controls.Add(this.label14);
            this.groupBox4.Controls.Add(this.label13);
            this.groupBox4.Controls.Add(this.label12);
            this.groupBox4.Controls.Add(this.label10);
            this.groupBox4.Controls.Add(this.panel5);
            this.groupBox4.Controls.Add(this.panel4);
            this.groupBox4.Controls.Add(this.label9);
            this.groupBox4.Controls.Add(this.minSizeBbl);
            this.groupBox4.Controls.Add(this.dataThres);
            this.groupBox4.Controls.Add(this.syscallThres);
            this.groupBox4.Controls.Add(this.DataCheckBox);
            this.groupBox4.Controls.Add(this.syscallCheckBox);
            this.groupBox4.Controls.Add(this.dataFlowCheckBox);
            this.groupBox4.Controls.Add(this.controlFlowCheckBox);
            this.groupBox4.Controls.Add(this.codeCheckBox);
            this.groupBox4.Controls.Add(this.orderCheckBox);
            this.groupBox4.Controls.Add(this.frequencyCheckBox);
            this.groupBox4.Controls.Add(this.panel3);
            this.groupBox4.Controls.Add(this.panel6);
            this.groupBox4.Controls.Add(this.panel2);
            this.groupBox4.Controls.Add(this.panel1);
            this.groupBox4.Controls.Add(this.dfgThreshold);
            this.groupBox4.Controls.Add(this.execThres);
            this.groupBox4.Controls.Add(this.orderThres);
            this.groupBox4.Controls.Add(this.dfgLevel);
            this.groupBox4.Controls.Add(this.cfgThreshold);
            this.groupBox4.Controls.Add(this.cfgLevel);
            this.groupBox4.Controls.Add(this.insThres);
            this.groupBox4.Controls.Add(this.GuessMapping);
            this.groupBox4.Location = new System.Drawing.Point(580, 115);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(303, 415);
            this.groupBox4.TabIndex = 56;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Guess";
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label18.Location = new System.Drawing.Point(201, 242);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(49, 13);
            this.label18.TabIndex = 77;
            this.label18.Text = "Direction";
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label17.Location = new System.Drawing.Point(121, 242);
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
            this.label16.Location = new System.Drawing.Point(73, 190);
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
            this.label15.Location = new System.Drawing.Point(6, 190);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(48, 13);
            this.label15.TabIndex = 77;
            this.label15.Text = "Classifier";
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label14.Location = new System.Drawing.Point(6, 16);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(40, 13);
            this.label14.TabIndex = 76;
            this.label14.Text = "Phase:";
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label13.Location = new System.Drawing.Point(6, 58);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(115, 13);
            this.label13.TabIndex = 75;
            this.label13.Text = "When to add mapping:";
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label12.Location = new System.Drawing.Point(6, 100);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(130, 13);
            this.label12.TabIndex = 74;
            this.label12.Text = "Which blocks to consider:";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label10.Location = new System.Drawing.Point(6, 139);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(60, 13);
            this.label10.TabIndex = 73;
            this.label10.Text = "Granularity:";
            // 
            // panel5
            // 
            this.panel5.Controls.Add(this.dfBoth);
            this.panel5.Controls.Add(this.dfDown);
            this.panel5.Controls.Add(this.dfUp);
            this.panel5.Location = new System.Drawing.Point(164, 284);
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
            this.panel4.Controls.Add(this.cfBoth);
            this.panel4.Controls.Add(this.cfDown);
            this.panel4.Controls.Add(this.cfUp);
            this.panel4.Location = new System.Drawing.Point(164, 261);
            this.panel4.Name = "panel4";
            this.panel4.Size = new System.Drawing.Size(138, 22);
            this.panel4.TabIndex = 71;
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
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(205, 119);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(61, 13);
            this.label9.TabIndex = 70;
            this.label9.Text = "minBblSize:";
            // 
            // minSizeBbl
            // 
            this.minSizeBbl.Location = new System.Drawing.Point(267, 116);
            this.minSizeBbl.Name = "minSizeBbl";
            this.minSizeBbl.Size = new System.Drawing.Size(31, 20);
            this.minSizeBbl.TabIndex = 68;
            // 
            // dataThres
            // 
            this.dataThres.Location = new System.Drawing.Point(86, 239);
            this.dataThres.Name = "dataThres";
            this.dataThres.Size = new System.Drawing.Size(31, 20);
            this.dataThres.TabIndex = 67;
            // 
            // syscallThres
            // 
            this.syscallThres.Location = new System.Drawing.Point(86, 309);
            this.syscallThres.Name = "syscallThres";
            this.syscallThres.Size = new System.Drawing.Size(31, 20);
            this.syscallThres.TabIndex = 67;
            // 
            // DataCheckBox
            // 
            this.DataCheckBox.AutoSize = true;
            this.DataCheckBox.Location = new System.Drawing.Point(9, 241);
            this.DataCheckBox.Name = "DataCheckBox";
            this.DataCheckBox.Size = new System.Drawing.Size(49, 17);
            this.DataCheckBox.TabIndex = 66;
            this.DataCheckBox.Text = "Data";
            this.DataCheckBox.UseVisualStyleBackColor = true;
            // 
            // syscallCheckBox
            // 
            this.syscallCheckBox.AutoSize = true;
            this.syscallCheckBox.Location = new System.Drawing.Point(9, 311);
            this.syscallCheckBox.Name = "syscallCheckBox";
            this.syscallCheckBox.Size = new System.Drawing.Size(64, 17);
            this.syscallCheckBox.TabIndex = 66;
            this.syscallCheckBox.Text = "Syscalls";
            this.syscallCheckBox.UseVisualStyleBackColor = true;
            // 
            // dataFlowCheckBox
            // 
            this.dataFlowCheckBox.AutoSize = true;
            this.dataFlowCheckBox.Location = new System.Drawing.Point(9, 288);
            this.dataFlowCheckBox.Name = "dataFlowCheckBox";
            this.dataFlowCheckBox.Size = new System.Drawing.Size(40, 17);
            this.dataFlowCheckBox.TabIndex = 65;
            this.dataFlowCheckBox.Text = "DF";
            this.dataFlowCheckBox.UseVisualStyleBackColor = true;
            // 
            // controlFlowCheckBox
            // 
            this.controlFlowCheckBox.AutoSize = true;
            this.controlFlowCheckBox.Location = new System.Drawing.Point(9, 265);
            this.controlFlowCheckBox.Name = "controlFlowCheckBox";
            this.controlFlowCheckBox.Size = new System.Drawing.Size(39, 17);
            this.controlFlowCheckBox.TabIndex = 64;
            this.controlFlowCheckBox.Text = "CF";
            this.controlFlowCheckBox.UseVisualStyleBackColor = true;
            // 
            // codeCheckBox
            // 
            this.codeCheckBox.AutoSize = true;
            this.codeCheckBox.Location = new System.Drawing.Point(9, 218);
            this.codeCheckBox.Name = "codeCheckBox";
            this.codeCheckBox.Size = new System.Drawing.Size(51, 17);
            this.codeCheckBox.TabIndex = 63;
            this.codeCheckBox.Text = "Code";
            this.codeCheckBox.UseVisualStyleBackColor = true;
            // 
            // orderCheckBox
            // 
            this.orderCheckBox.AutoSize = true;
            this.orderCheckBox.Location = new System.Drawing.Point(9, 334);
            this.orderCheckBox.Name = "orderCheckBox";
            this.orderCheckBox.Size = new System.Drawing.Size(52, 17);
            this.orderCheckBox.TabIndex = 62;
            this.orderCheckBox.Text = "Order";
            this.orderCheckBox.UseVisualStyleBackColor = true;
            // 
            // frequencyCheckBox
            // 
            this.frequencyCheckBox.AutoSize = true;
            this.frequencyCheckBox.Location = new System.Drawing.Point(9, 357);
            this.frequencyCheckBox.Name = "frequencyCheckBox";
            this.frequencyCheckBox.Size = new System.Drawing.Size(76, 17);
            this.frequencyCheckBox.TabIndex = 61;
            this.frequencyCheckBox.Text = "Frequency";
            this.frequencyCheckBox.UseVisualStyleBackColor = true;
            // 
            // panel3
            // 
            this.panel3.Controls.Add(this.matchAllMatches);
            this.panel3.Controls.Add(this.matchBestOrUnmatchWorst);
            this.panel3.Controls.Add(this.matchFirst);
            this.panel3.Location = new System.Drawing.Point(9, 74);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(293, 23);
            this.panel3.TabIndex = 60;
            // 
            // matchAllMatches
            // 
            this.matchAllMatches.AutoSize = true;
            this.matchAllMatches.Location = new System.Drawing.Point(212, 0);
            this.matchAllMatches.Name = "matchAllMatches";
            this.matchAllMatches.Size = new System.Drawing.Size(35, 17);
            this.matchAllMatches.TabIndex = 56;
            this.matchAllMatches.TabStop = true;
            this.matchAllMatches.Text = "all";
            this.matchAllMatches.UseVisualStyleBackColor = true;
            // 
            // matchBestOrUnmatchWorst
            // 
            this.matchBestOrUnmatchWorst.AutoSize = true;
            this.matchBestOrUnmatchWorst.Location = new System.Drawing.Point(110, 0);
            this.matchBestOrUnmatchWorst.Name = "matchBestOrUnmatchWorst";
            this.matchBestOrUnmatchWorst.Size = new System.Drawing.Size(45, 17);
            this.matchBestOrUnmatchWorst.TabIndex = 55;
            this.matchBestOrUnmatchWorst.TabStop = true;
            this.matchBestOrUnmatchWorst.Text = "best";
            this.matchBestOrUnmatchWorst.UseVisualStyleBackColor = true;
            // 
            // matchFirst
            // 
            this.matchFirst.AutoSize = true;
            this.matchFirst.Location = new System.Drawing.Point(4, 0);
            this.matchFirst.Name = "matchFirst";
            this.matchFirst.Size = new System.Drawing.Size(41, 17);
            this.matchFirst.TabIndex = 54;
            this.matchFirst.TabStop = true;
            this.matchFirst.Text = "first";
            this.matchFirst.UseVisualStyleBackColor = true;
            // 
            // panel6
            // 
            this.panel6.Controls.Add(this.TransitionLevel);
            this.panel6.Controls.Add(this.InstructionLevel);
            this.panel6.Controls.Add(this.BasicBlockLevel);
            this.panel6.Location = new System.Drawing.Point(9, 155);
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
            this.TransitionLevel.TabIndex = 59;
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
            this.InstructionLevel.TabIndex = 58;
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
            this.BasicBlockLevel.TabIndex = 57;
            this.BasicBlockLevel.TabStop = true;
            this.BasicBlockLevel.Text = "basic block";
            this.BasicBlockLevel.UseVisualStyleBackColor = true;
            // 
            // panel2
            // 
            this.panel2.Controls.Add(this.tryAll);
            this.panel2.Controls.Add(this.tryOnlyUnmapped);
            this.panel2.Location = new System.Drawing.Point(9, 116);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(193, 20);
            this.panel2.TabIndex = 60;
            // 
            // tryAll
            // 
            this.tryAll.AutoSize = true;
            this.tryAll.Location = new System.Drawing.Point(110, 1);
            this.tryAll.Name = "tryAll";
            this.tryAll.Size = new System.Drawing.Size(35, 17);
            this.tryAll.TabIndex = 58;
            this.tryAll.TabStop = true;
            this.tryAll.Text = "all";
            this.tryAll.UseVisualStyleBackColor = true;
            // 
            // tryOnlyUnmapped
            // 
            this.tryOnlyUnmapped.AutoSize = true;
            this.tryOnlyUnmapped.Location = new System.Drawing.Point(4, 1);
            this.tryOnlyUnmapped.Name = "tryOnlyUnmapped";
            this.tryOnlyUnmapped.Size = new System.Drawing.Size(75, 17);
            this.tryOnlyUnmapped.TabIndex = 57;
            this.tryOnlyUnmapped.TabStop = true;
            this.tryOnlyUnmapped.Text = "unmapped";
            this.tryOnlyUnmapped.UseVisualStyleBackColor = true;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.filter);
            this.panel1.Controls.Add(this.extend);
            this.panel1.Controls.Add(this.initial);
            this.panel1.Location = new System.Drawing.Point(9, 32);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(293, 23);
            this.panel1.TabIndex = 59;
            // 
            // filter
            // 
            this.filter.AutoSize = true;
            this.filter.Location = new System.Drawing.Point(212, 0);
            this.filter.Name = "filter";
            this.filter.Size = new System.Drawing.Size(44, 17);
            this.filter.TabIndex = 56;
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
            this.extend.TabIndex = 55;
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
            this.initial.TabIndex = 54;
            this.initial.TabStop = true;
            this.initial.Text = "initial";
            this.initial.UseVisualStyleBackColor = true;
            this.initial.CheckedChanged += new System.EventHandler(this.initial_CheckedChanged);
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.scoreFP);
            this.groupBox5.Controls.Add(this.scoreFN);
            this.groupBox5.Controls.Add(this.scoreOK);
            this.groupBox5.Controls.Add(this.scoreGOAL);
            this.groupBox5.Location = new System.Drawing.Point(12, 697);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Size = new System.Drawing.Size(180, 83);
            this.groupBox5.TabIndex = 57;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "Score";
            // 
            // scoreFP
            // 
            this.scoreFP.AutoSize = true;
            this.scoreFP.Location = new System.Drawing.Point(6, 29);
            this.scoreFP.Name = "scoreFP";
            this.scoreFP.Size = new System.Drawing.Size(30, 13);
            this.scoreFP.TabIndex = 43;
            this.scoreFP.Text = "#FP:";
            // 
            // scoreFN
            // 
            this.scoreFN.AutoSize = true;
            this.scoreFN.Location = new System.Drawing.Point(6, 42);
            this.scoreFN.Name = "scoreFN";
            this.scoreFN.Size = new System.Drawing.Size(31, 13);
            this.scoreFN.TabIndex = 42;
            this.scoreFN.Text = "#FN:";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label7.Location = new System.Drawing.Point(9, 6);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(55, 13);
            this.label7.TabIndex = 58;
            this.label7.Text = "Program 1";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label8.Location = new System.Drawing.Point(293, 6);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(55, 13);
            this.label8.TabIndex = 59;
            this.label8.Text = "Program 2";
            // 
            // groupBox6
            // 
            this.groupBox6.Controls.Add(this.RemoveSettings);
            this.groupBox6.Controls.Add(this.appliedAllCheckbox);
            this.groupBox6.Controls.Add(this.toApplyCheckBoxAll);
            this.groupBox6.Controls.Add(this.label20);
            this.groupBox6.Controls.Add(this.LoadSettingsButton);
            this.groupBox6.Controls.Add(this.toApplyListBox);
            this.groupBox6.Controls.Add(this.SaveSettings);
            this.groupBox6.Controls.Add(this.copyToRight);
            this.groupBox6.Controls.Add(this.label19);
            this.groupBox6.Controls.Add(this.ApplySettings);
            this.groupBox6.Controls.Add(this.AppliedSettingsListBox);
            this.groupBox6.Location = new System.Drawing.Point(12, 536);
            this.groupBox6.Name = "groupBox6";
            this.groupBox6.Size = new System.Drawing.Size(871, 155);
            this.groupBox6.TabIndex = 60;
            this.groupBox6.TabStop = false;
            this.groupBox6.Text = "Guesses";
            // 
            // RemoveSettings
            // 
            this.RemoveSettings.Location = new System.Drawing.Point(813, 92);
            this.RemoveSettings.Name = "RemoveSettings";
            this.RemoveSettings.Size = new System.Drawing.Size(49, 26);
            this.RemoveSettings.TabIndex = 69;
            this.RemoveSettings.Text = "X";
            this.RemoveSettings.UseVisualStyleBackColor = true;
            this.RemoveSettings.Click += new System.EventHandler(this.RemoveSettingsFromListButton_Click);
            // 
            // appliedAllCheckbox
            // 
            this.appliedAllCheckbox.AutoSize = true;
            this.appliedAllCheckbox.Location = new System.Drawing.Point(392, 124);
            this.appliedAllCheckbox.Name = "appliedAllCheckbox";
            this.appliedAllCheckbox.Size = new System.Drawing.Size(15, 14);
            this.appliedAllCheckbox.TabIndex = 68;
            this.appliedAllCheckbox.UseVisualStyleBackColor = true;
            this.appliedAllCheckbox.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
            // 
            // toApplyCheckBoxAll
            // 
            this.toApplyCheckBoxAll.AutoSize = true;
            this.toApplyCheckBoxAll.Location = new System.Drawing.Point(813, 124);
            this.toApplyCheckBoxAll.Name = "toApplyCheckBoxAll";
            this.toApplyCheckBoxAll.Size = new System.Drawing.Size(15, 14);
            this.toApplyCheckBoxAll.TabIndex = 67;
            this.toApplyCheckBoxAll.UseVisualStyleBackColor = true;
            this.toApplyCheckBoxAll.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
            // 
            // label20
            // 
            this.label20.AutoSize = true;
            this.label20.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label20.Location = new System.Drawing.Point(444, 12);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(49, 13);
            this.label20.TabIndex = 66;
            this.label20.Text = "To Apply";
            // 
            // LoadSettingsButton
            // 
            this.LoadSettingsButton.Location = new System.Drawing.Point(813, 28);
            this.LoadSettingsButton.Name = "LoadSettingsButton";
            this.LoadSettingsButton.Size = new System.Drawing.Size(49, 26);
            this.LoadSettingsButton.TabIndex = 65;
            this.LoadSettingsButton.Text = "Load";
            this.LoadSettingsButton.UseVisualStyleBackColor = true;
            this.LoadSettingsButton.Click += new System.EventHandler(this.LoadSettings_Click);
            // 
            // toApplyListBox
            // 
            this.toApplyListBox.CheckOnClick = true;
            this.toApplyListBox.FormattingEnabled = true;
            this.toApplyListBox.HorizontalScrollbar = true;
            this.toApplyListBox.Location = new System.Drawing.Point(447, 28);
            this.toApplyListBox.Name = "toApplyListBox";
            this.toApplyListBox.Size = new System.Drawing.Size(360, 109);
            this.toApplyListBox.TabIndex = 64;
            // 
            // SaveSettings
            // 
            this.SaveSettings.Location = new System.Drawing.Point(392, 28);
            this.SaveSettings.Name = "SaveSettings";
            this.SaveSettings.Size = new System.Drawing.Size(49, 26);
            this.SaveSettings.TabIndex = 63;
            this.SaveSettings.Text = "Save";
            this.SaveSettings.UseVisualStyleBackColor = true;
            this.SaveSettings.Click += new System.EventHandler(this.SaveSettings_Click);
            // 
            // copyToRight
            // 
            this.copyToRight.Location = new System.Drawing.Point(392, 60);
            this.copyToRight.Name = "copyToRight";
            this.copyToRight.Size = new System.Drawing.Size(49, 26);
            this.copyToRight.TabIndex = 62;
            this.copyToRight.Text = "-->";
            this.copyToRight.UseVisualStyleBackColor = true;
            this.copyToRight.Click += new System.EventHandler(this.copyToRight_Click);
            // 
            // label19
            // 
            this.label19.AutoSize = true;
            this.label19.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label19.Location = new System.Drawing.Point(7, 12);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(42, 13);
            this.label19.TabIndex = 61;
            this.label19.Text = "Applied";
            // 
            // ApplySettings
            // 
            this.ApplySettings.Location = new System.Drawing.Point(813, 60);
            this.ApplySettings.Name = "ApplySettings";
            this.ApplySettings.Size = new System.Drawing.Size(49, 26);
            this.ApplySettings.TabIndex = 2;
            this.ApplySettings.Text = "Apply";
            this.ApplySettings.UseVisualStyleBackColor = true;
            this.ApplySettings.Click += new System.EventHandler(this.ApplySettings_Click);
            // 
            // AppliedSettingsListBox
            // 
            this.AppliedSettingsListBox.CheckOnClick = true;
            this.AppliedSettingsListBox.FormattingEnabled = true;
            this.AppliedSettingsListBox.HorizontalScrollbar = true;
            this.AppliedSettingsListBox.Location = new System.Drawing.Point(10, 28);
            this.AppliedSettingsListBox.Name = "AppliedSettingsListBox";
            this.AppliedSettingsListBox.Size = new System.Drawing.Size(376, 109);
            this.AppliedSettingsListBox.TabIndex = 0;
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.FileName = "openFileDialog1";
            // 
            // saveFileDialog1
            // 
            this.saveFileDialog1.DefaultExt = "settings";
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(684, 524);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(47, 27);
            this.button2.TabIndex = 78;
            this.button2.Text = "Up";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.loadSettingsButton_Click);
            // 
            // button3
            // 
            this.button3.Location = new System.Drawing.Point(735, 524);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(47, 27);
            this.button3.TabIndex = 79;
            this.button3.Text = "Down";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Click += new System.EventHandler(this.MoveSettingsToListButton_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(891, 851);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.button3);
            this.Controls.Add(this.groupBox6);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.groupBox5);
            this.Controls.Add(this.groupBox4);
            this.Controls.Add(this.groupBox3);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.swap);
            this.Controls.Add(this.Program2);
            this.Controls.Add(this.consoleOutput);
            this.Controls.Add(this.Program1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "Form1";
            this.Text = "Evaluating Diversity";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.groupBox4.ResumeLayout(false);
            this.groupBox4.PerformLayout();
            this.panel5.ResumeLayout(false);
            this.panel5.PerformLayout();
            this.panel4.ResumeLayout(false);
            this.panel4.PerformLayout();
            this.panel3.ResumeLayout(false);
            this.panel3.PerformLayout();
            this.panel6.ResumeLayout(false);
            this.panel6.PerformLayout();
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.groupBox5.ResumeLayout(false);
            this.groupBox5.PerformLayout();
            this.groupBox6.ResumeLayout(false);
            this.groupBox6.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }
        #endregion
        private System.Windows.Forms.ListBox Program1;
        public System.Windows.Forms.TextBox consoleOutput;
        private System.Windows.Forms.ListBox Program2;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ListBox falseNegativesCombined;
        private System.Windows.Forms.ListBox falsePositives;
        private System.Windows.Forms.ListBox falseNegatives;
        private System.Windows.Forms.ListBox Ok;
        private System.Windows.Forms.Button GuessMapping;
        private System.Windows.Forms.Label scoreGOAL;
        private System.Windows.Forms.FolderBrowserDialog folderBrowserDialog1;
        private System.Windows.Forms.FolderBrowserDialog folderBrowserDialog2;
        private System.Windows.Forms.Button loadProgram1;
        private System.Windows.Forms.TextBox program1TextBox;
        private System.Windows.Forms.TextBox program2TextBox;
        private System.Windows.Forms.Button loadProgram2;
        private System.Windows.Forms.Button load;
        private System.Windows.Forms.ListBox correct;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Button swap;
        private System.Windows.Forms.Label scoreOK;
        private System.Windows.Forms.TextBox insThres;
        private System.Windows.Forms.TextBox orderThres;
        private System.Windows.Forms.TextBox execThres;
        private System.Windows.Forms.ListBox falsePositivesCombined;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.TextBox cfgLevel;
        private System.Windows.Forms.TextBox cfgThreshold;
        private System.Windows.Forms.TextBox dfgLevel;
        private System.Windows.Forms.TextBox dfgThreshold;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.GroupBox groupBox5;
        private System.Windows.Forms.Label scoreFP;
        private System.Windows.Forms.Label scoreFN;
        private System.Windows.Forms.RadioButton tryAll;
        private System.Windows.Forms.RadioButton tryOnlyUnmapped;
        private System.Windows.Forms.RadioButton filter;
        private System.Windows.Forms.RadioButton extend;
        private System.Windows.Forms.RadioButton initial;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.RadioButton matchAllMatches;
        private System.Windows.Forms.RadioButton matchBestOrUnmatchWorst;
        private System.Windows.Forms.RadioButton matchFirst;
        private System.Windows.Forms.CheckBox orderCheckBox;
        private System.Windows.Forms.CheckBox frequencyCheckBox;
        private System.Windows.Forms.CheckBox dataFlowCheckBox;
        private System.Windows.Forms.CheckBox controlFlowCheckBox;
        private System.Windows.Forms.CheckBox codeCheckBox;
        private System.Windows.Forms.TextBox dataThres;
        private System.Windows.Forms.TextBox syscallThres;
        private System.Windows.Forms.CheckBox DataCheckBox;
        private System.Windows.Forms.CheckBox syscallCheckBox;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.TextBox minSizeBbl;
        private System.Windows.Forms.Label label9;
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
        private System.Windows.Forms.GroupBox groupBox6;
        private System.Windows.Forms.Button ApplySettings;
        private System.Windows.Forms.CheckedListBox AppliedSettingsListBox;
        private System.Windows.Forms.Label label19;
        private System.Windows.Forms.Button copyToRight;
        private System.Windows.Forms.Button LoadSettingsButton;
        private System.Windows.Forms.CheckedListBox toApplyListBox;
        private System.Windows.Forms.Button SaveSettings;
        private System.Windows.Forms.Label label20;
        private System.Windows.Forms.CheckBox appliedAllCheckbox;
        private System.Windows.Forms.CheckBox toApplyCheckBoxAll;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.SaveFileDialog saveFileDialog1;
        private System.Windows.Forms.Button RemoveSettings;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.RadioButton InstructionLevel;
        private System.Windows.Forms.RadioButton TransitionLevel;
    }
}
