namespace EvaluateDiversity
{
    partial class InspectionForm
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
            this.label8 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.tableLayoutPanel3 = new System.Windows.Forms.TableLayoutPanel();
            this.label1 = new System.Windows.Forms.Label();
            this.OkCombined = new System.Windows.Forms.ListBox();
            this.label5 = new System.Windows.Forms.Label();
            this.falsePositivesCombined = new System.Windows.Forms.ListBox();
            this.falseNegativesCombined = new System.Windows.Forms.ListBox();
            this.label11 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.label2 = new System.Windows.Forms.Label();
            this.Ok = new System.Windows.Forms.ListBox();
            this.falsePositives = new System.Windows.Forms.ListBox();
            this.label4 = new System.Windows.Forms.Label();
            this.falseNegatives = new System.Windows.Forms.ListBox();
            this.label3 = new System.Windows.Forms.Label();
            this.swap = new System.Windows.Forms.Button();
            this.Program2 = new System.Windows.Forms.ListBox();
            this.Program1 = new System.Windows.Forms.ListBox();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.consoleOutput = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.groupBox2.SuspendLayout();
            this.tableLayoutPanel3.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.tableLayoutPanel1.SetColumnSpan(this.label8, 2);
            this.label8.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label8.Location = new System.Drawing.Point(343, 0);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(55, 13);
            this.label8.TabIndex = 66;
            this.label8.Text = "Program 2";
            this.label8.TextAlign = System.Drawing.ContentAlignment.BottomLeft;
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label7.Location = new System.Drawing.Point(3, 0);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(55, 13);
            this.label7.TabIndex = 65;
            this.label7.Text = "Program 1";
            // 
            // groupBox2
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.groupBox2, 2);
            this.groupBox2.Controls.Add(this.tableLayoutPanel3);
            this.groupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox2.Location = new System.Drawing.Point(3, 381);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(334, 322);
            this.groupBox2.TabIndex = 64;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Entire Program";
            // 
            // tableLayoutPanel3
            // 
            this.tableLayoutPanel3.ColumnCount = 1;
            this.tableLayoutPanel3.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel3.Controls.Add(this.label1, 0, 0);
            this.tableLayoutPanel3.Controls.Add(this.OkCombined, 0, 5);
            this.tableLayoutPanel3.Controls.Add(this.label5, 0, 4);
            this.tableLayoutPanel3.Controls.Add(this.falsePositivesCombined, 0, 3);
            this.tableLayoutPanel3.Controls.Add(this.falseNegativesCombined, 0, 1);
            this.tableLayoutPanel3.Controls.Add(this.label11, 0, 2);
            this.tableLayoutPanel3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel3.Location = new System.Drawing.Point(3, 16);
            this.tableLayoutPanel3.Name = "tableLayoutPanel3";
            this.tableLayoutPanel3.RowCount = 6;
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel3.Size = new System.Drawing.Size(328, 303);
            this.tableLayoutPanel3.TabIndex = 50;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(83, 13);
            this.label1.TabIndex = 17;
            this.label1.Text = "False Negatives";
            // 
            // OkCombined
            // 
            this.OkCombined.Dock = System.Windows.Forms.DockStyle.Fill;
            this.OkCombined.FormattingEnabled = true;
            this.OkCombined.Location = new System.Drawing.Point(3, 225);
            this.OkCombined.Name = "OkCombined";
            this.OkCombined.Size = new System.Drawing.Size(322, 69);
            this.OkCombined.TabIndex = 37;
            this.OkCombined.SelectedIndexChanged += new System.EventHandler(this.correct_SelectedIndexChanged);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(3, 202);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(22, 13);
            this.label5.TabIndex = 38;
            this.label5.Text = "OK";
            // 
            // falsePositivesCombined
            // 
            this.falsePositivesCombined.Dock = System.Windows.Forms.DockStyle.Fill;
            this.falsePositivesCombined.FormattingEnabled = true;
            this.falsePositivesCombined.Location = new System.Drawing.Point(3, 124);
            this.falsePositivesCombined.Name = "falsePositivesCombined";
            this.falsePositivesCombined.Size = new System.Drawing.Size(322, 69);
            this.falsePositivesCombined.TabIndex = 49;
            this.falsePositivesCombined.SelectedIndexChanged += new System.EventHandler(this.falsePositivesCombined_SelectedIndexChanged);
            // 
            // falseNegativesCombined
            // 
            this.falseNegativesCombined.Dock = System.Windows.Forms.DockStyle.Fill;
            this.falseNegativesCombined.FormattingEnabled = true;
            this.falseNegativesCombined.Location = new System.Drawing.Point(3, 23);
            this.falseNegativesCombined.Name = "falseNegativesCombined";
            this.falseNegativesCombined.Size = new System.Drawing.Size(322, 69);
            this.falseNegativesCombined.TabIndex = 20;
            this.falseNegativesCombined.SelectedIndexChanged += new System.EventHandler(this.falseNegativesCombined_SelectedIndexChanged);
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(3, 101);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(77, 13);
            this.label11.TabIndex = 48;
            this.label11.Text = "False Positives";
            // 
            // groupBox1
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.groupBox1, 2);
            this.groupBox1.Controls.Add(this.tableLayoutPanel2);
            this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox1.Location = new System.Drawing.Point(343, 381);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(335, 322);
            this.groupBox1.TabIndex = 63;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Selected Instruction";
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.ColumnCount = 1;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.Controls.Add(this.label2, 0, 0);
            this.tableLayoutPanel2.Controls.Add(this.Ok, 0, 5);
            this.tableLayoutPanel2.Controls.Add(this.falsePositives, 0, 1);
            this.tableLayoutPanel2.Controls.Add(this.label4, 0, 4);
            this.tableLayoutPanel2.Controls.Add(this.falseNegatives, 0, 3);
            this.tableLayoutPanel2.Controls.Add(this.label3, 0, 2);
            this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel2.Location = new System.Drawing.Point(3, 16);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 6;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(329, 303);
            this.tableLayoutPanel2.TabIndex = 24;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(3, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(77, 13);
            this.label2.TabIndex = 7;
            this.label2.Text = "False Positives";
            // 
            // Ok
            // 
            this.Ok.Dock = System.Windows.Forms.DockStyle.Fill;
            this.Ok.FormattingEnabled = true;
            this.Ok.Location = new System.Drawing.Point(3, 225);
            this.Ok.Name = "Ok";
            this.Ok.Size = new System.Drawing.Size(323, 69);
            this.Ok.TabIndex = 23;
            this.Ok.SelectedIndexChanged += new System.EventHandler(this.Ok_SelectedIndexChanged);
            // 
            // falsePositives
            // 
            this.falsePositives.Dock = System.Windows.Forms.DockStyle.Fill;
            this.falsePositives.FormattingEnabled = true;
            this.falsePositives.Location = new System.Drawing.Point(3, 23);
            this.falsePositives.Name = "falsePositives";
            this.falsePositives.Size = new System.Drawing.Size(323, 69);
            this.falsePositives.TabIndex = 21;
            this.falsePositives.SelectedIndexChanged += new System.EventHandler(this.falsePositives_SelectedIndexChanged);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(3, 202);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(22, 13);
            this.label4.TabIndex = 13;
            this.label4.Text = "OK";
            // 
            // falseNegatives
            // 
            this.falseNegatives.Dock = System.Windows.Forms.DockStyle.Fill;
            this.falseNegatives.FormattingEnabled = true;
            this.falseNegatives.Location = new System.Drawing.Point(3, 124);
            this.falseNegatives.Name = "falseNegatives";
            this.falseNegatives.Size = new System.Drawing.Size(323, 69);
            this.falseNegatives.TabIndex = 22;
            this.falseNegatives.SelectedIndexChanged += new System.EventHandler(this.falseNegatives_SelectedIndexChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(3, 101);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(83, 13);
            this.label3.TabIndex = 10;
            this.label3.Text = "False Negatives";
            // 
            // swap
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.swap, 2);
            this.swap.Location = new System.Drawing.Point(308, 351);
            this.swap.Name = "swap";
            this.swap.Size = new System.Drawing.Size(64, 24);
            this.swap.TabIndex = 62;
            this.swap.Text = "Swap";
            this.swap.UseVisualStyleBackColor = true;
            // 
            // Program2
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.Program2, 2);
            this.Program2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.Program2.DrawMode = System.Windows.Forms.DrawMode.OwnerDrawFixed;
            this.Program2.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Program2.FormattingEnabled = true;
            this.Program2.ItemHeight = 14;
            this.Program2.Location = new System.Drawing.Point(343, 23);
            this.Program2.Name = "Program2";
            this.Program2.Size = new System.Drawing.Size(335, 312);
            this.Program2.TabIndex = 61;
            this.Program2.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.rightLstBox_DrawItem);
            this.Program2.SelectedIndexChanged += new System.EventHandler(this.Program2_SelectedIndexChanged);
            // 
            // Program1
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.Program1, 2);
            this.Program1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.Program1.DrawMode = System.Windows.Forms.DrawMode.OwnerDrawFixed;
            this.Program1.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Program1.FormattingEnabled = true;
            this.Program1.ItemHeight = 14;
            this.Program1.Location = new System.Drawing.Point(3, 23);
            this.Program1.Name = "Program1";
            this.Program1.Size = new System.Drawing.Size(334, 312);
            this.Program1.TabIndex = 60;
            this.Program1.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.lstBox_DrawItem);
            this.Program1.SelectedIndexChanged += new System.EventHandler(this.Program1_SelectedIndexChanged);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 5;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 35F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 35F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 33.33334F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 33.33334F));
            this.tableLayoutPanel1.Controls.Add(this.consoleOutput, 4, 1);
            this.tableLayoutPanel1.Controls.Add(this.label7, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.groupBox2, 0, 3);
            this.tableLayoutPanel1.Controls.Add(this.Program1, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.swap, 1, 2);
            this.tableLayoutPanel1.Controls.Add(this.Program2, 2, 1);
            this.tableLayoutPanel1.Controls.Add(this.groupBox1, 2, 3);
            this.tableLayoutPanel1.Controls.Add(this.label8, 2, 0);
            this.tableLayoutPanel1.Controls.Add(this.label6, 4, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 4;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 30F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(988, 706);
            this.tableLayoutPanel1.TabIndex = 67;
            // 
            // consoleOutput
            // 
            this.consoleOutput.Dock = System.Windows.Forms.DockStyle.Fill;
            this.consoleOutput.Location = new System.Drawing.Point(684, 23);
            this.consoleOutput.Multiline = true;
            this.consoleOutput.Name = "consoleOutput";
            this.tableLayoutPanel1.SetRowSpan(this.consoleOutput, 3);
            this.consoleOutput.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.consoleOutput.Size = new System.Drawing.Size(301, 680);
            this.consoleOutput.TabIndex = 68;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label6.Location = new System.Drawing.Point(684, 0);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(45, 13);
            this.label6.TabIndex = 65;
            this.label6.Text = "Console";
            // 
            // InspectionForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(988, 706);
            this.Controls.Add(this.tableLayoutPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.Name = "InspectionForm";
            this.Text = "Form2";
            this.VisibleChanged += new System.EventHandler(this.InspectionForm_VisibleChanged);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.InspectionForm_FormClosing);
            this.groupBox2.ResumeLayout(false);
            this.tableLayoutPanel3.ResumeLayout(false);
            this.tableLayoutPanel3.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.tableLayoutPanel2.ResumeLayout(false);
            this.tableLayoutPanel2.PerformLayout();
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.ListBox falseNegatives;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button swap;
        internal System.Windows.Forms.ListBox Program2;
        internal System.Windows.Forms.ListBox Program1;
        public System.Windows.Forms.ListBox falsePositivesCombined;
        public System.Windows.Forms.ListBox falseNegativesCombined;
        public System.Windows.Forms.ListBox falsePositives;
        public System.Windows.Forms.ListBox OkCombined;
        public System.Windows.Forms.ListBox Ok;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel3;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        public System.Windows.Forms.TextBox consoleOutput;
        private System.Windows.Forms.Label label6;
    }
}