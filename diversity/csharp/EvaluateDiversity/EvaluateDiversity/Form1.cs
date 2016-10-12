using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
namespace EvaluateDiversity
{
    public partial class Form1 : Form
    {
        Comparison comparison;
        public Instruction currentInstructionProgram1
        {
            get { return (Instruction)Program1.SelectedItem; }
        }
        public Instruction currentInstructionProgram2
        {
            get { return (Instruction)Program2.SelectedItem; }
        }
        private void lstBox_DrawItem(object sender, System.Windows.Forms.DrawItemEventArgs e)
        {
            //
            // Draw the background of the ListBox control for each item.
            // Create a new Brush and initialize to a Black colored brush by default.
            //
            e.DrawBackground();
            Brush myBrush = Brushes.Black;
            //
            // Determine the color of the brush to draw each item based on 
            // the index of the item to draw.
            //
            if (e.Index % 2 == 0)
                myBrush = Brushes.Red;
            else myBrush = Brushes.Green;
            //
            // Draw the current item text based on the current Font and the custom brush settings.
            //
            if (e.Index != -1)
            {
                if (((Instruction)((ListBox)sender).Items[e.Index]).correct)
                    myBrush = Brushes.Green;
                else myBrush = Brushes.Red;
                e.Graphics.DrawString(((ListBox)sender).Items[e.Index].ToString(),
                            e.Font, myBrush, e.Bounds, StringFormat.GenericDefault);
                //
                // If the ListBox has focus, draw a focus rectangle around the selected item.
                //
                e.DrawFocusRectangle();
            }
        }
        private void rightLstBox_DrawItem(object sender, System.Windows.Forms.DrawItemEventArgs e)
        {
            //
            // Draw the background of the ListBox control for each item.
            // Create a new Brush and initialize to a Black colored brush by default.
            //
            e.DrawBackground();
            Brush myBrush = Brushes.Black;
            //
            // Draw the current item text based on the current Font and the custom brush settings.
            //
            if (e.Index != -1)
            {
                if (currentInstructionProgram1 != null)
                {
                    if (((Instruction)((ListBox)sender).Items[e.Index]).getFalseNegatives().Contains(currentInstructionProgram1))
                        myBrush = Brushes.Red;
                    else if (((Instruction)((ListBox)sender).Items[e.Index]).getFalsePositives().Contains(currentInstructionProgram1))
                        myBrush = Brushes.Red;
                    else if (((Instruction)((ListBox)sender).Items[e.Index]).getOk().Contains(currentInstructionProgram1))
                        myBrush = Brushes.Green;
                    else
                        myBrush = Brushes.LightGray;
                }
                e.Graphics.DrawString(((ListBox)sender).Items[e.Index].ToString(),
                            e.Font, myBrush, e.Bounds, StringFormat.GenericDefault);
                //
                // If the ListBox has focus, draw a focus rectangle around the selected item.
                //
                e.DrawFocusRectangle();
            }
        }

        public Form1()
        {
            InitializeComponent();
            LoadSettings(Settings.Default);
        }

        private void LoadSettings(Settings settings)
        {
            /*checkboxes*/
            codeCheckBox.Checked = settings.code;
            frequencyCheckBox.Checked = settings.frequency;
            orderCheckBox.Checked = settings.order;
            dataFlowCheckBox.Checked = settings.dataFlow;
            controlFlowCheckBox.Checked = settings.controlFlow;
            DataCheckBox.Checked = settings.data;
            syscallCheckBox.Checked = settings.syscalls;

            /*thresholds*/
            insThres.Text = settings.insThreshold.ToString();
            orderThres.Text = settings.orderThreshold.ToString();
            execThres.Text = settings.execThreshold.ToString();
            cfgThreshold.Text = settings.cfgThreshold.ToString();
            dfgThreshold.Text = settings.dfgThreshold.ToString();
            dataThres.Text = settings.dataThreshold.ToString();
            syscallThres.Text = settings.syscallThreshold.ToString();

            /*minbbl*/
            minSizeBbl.Text = settings.minSizeBbl.ToString();

            /*phase*/
            if (settings.phase == Phase.INIT)
                initial.Checked = true;
            else if (settings.phase == Phase.FILTER)
                filter.Checked = true;
            else extend.Checked = true;

            /*whenToMatch*/
            if (settings.whenToMatch == WhenToMatchOrUnmatch.ALL)
                matchAllMatches.Checked = true;
            else if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST)
                matchBestOrUnmatchWorst.Checked = true;
            else
                matchFirst.Checked = true;

            /*only unmapped?*/
            if (settings.only_unmatched)
                tryOnlyUnmapped.Checked = true;
            else
                tryAll.Checked = true;

            /*parameters cfg and dfg*/
            dfgLevel.Text = settings.dfgLevel.ToString();
            cfgLevel.Text = settings.cfgLevel.ToString();
            switch (settings.cfDirection)
            {
                case Direction.UP:
                    cfUp.Checked = true;
                    break;
                case Direction.DOWN:
                    cfDown.Checked = true;
                    break;
                case Direction.BOTH:
                    cfBoth.Checked = true;
                    break;
                default:
                    throw new Exception();
            }
            switch (settings.dfDirection)
            {
                case Direction.UP:
                    dfUp.Checked = true;
                    break;
                case Direction.DOWN:
                    dfDown.Checked = true;
                    break;
                case Direction.BOTH:
                    dfBoth.Checked = true;
                    break;
                default:
                    throw new Exception();
            }

            /*granularity*/
            if (settings.granularity == Granularity.BASICBLOCK)
                BasicBlockLevel.Checked = true;
            else if (settings.granularity == Granularity.INSTRUCTION)
                InstructionLevel.Checked = true;
            else if (settings.granularity == Granularity.TRANSITION)
                TransitionLevel.Checked = true;
            else throw new Exception();

        }
        /*
        private void loadReference_Click(object sender, EventArgs e)
        {
            openFileDialogReference.ShowDialog();
        }
        private void loadMappingReference_Click(object sender, EventArgs e)
        {
            openFileDialogMappingReference.ShowDialog();
        }
        private void loadComparison_Click(object sender, EventArgs e)
        {
            openFileDialogComparison.ShowDialog();
        }
        private void loadMappingComparison_Click(object sender, EventArgs e)
        {
            openFileDialogMappingComparison.ShowDialog();
        }
        private void openFileDialogReference_FileOk(object sender, CancelEventArgs e)
        {
            textBoxReference.Text = openFileDialogReference.FileName;
            comparison.RegisterProgram1(textBoxReference.Text);
            ReloadLeft();
            loadMappingReference.Enabled = true;
            loadReference.Enabled = false;
        }*/
        private void ReloadLeft()
        {
            Program1.Items.Clear();
            int i = 0;
            foreach (KeyValuePair<Int32, Instruction> pair in comparison.program1)
            {
                Program1.Items.Add(pair.Value);
                pair.Value.index = i++;
            }
        }

        private void ReloadRight()
        {
            Program2.Items.Clear();
            int i = 0;
            foreach (KeyValuePair<Int32, Instruction> pair in comparison.program2)
            {
                Program2.Items.Add(pair.Value);
                pair.Value.index = i++;
            }
        }
        /*
        private void openFileDialogComparison_FileOk(object sender, CancelEventArgs e)
        {
            textBoxComparison.Text = openFileDialogComparison.FileName;
            comparison.RegisterProgram2(textBoxComparison.Text);
            Program1.Refresh();
            ReloadRight();
            ReloadFaulty();
            loadComparison.Enabled = false;
            loadMappingComparison.Enabled = true;
        }*/
        private void ReloadAllFalseNegatives()
        {
            falseNegativesCombined.Items.Clear();
            //TODO
            foreach (Instruction i in comparison.getAllFalseNegatives())
                if (i.basicBlock == null || i.basicBlock.instructions.Count < 3)
                    falseNegativesCombined.Items.Add(i);
        }

        private void ReloadAllFalsePositives()
        {
            falsePositivesCombined.Items.Clear();
            foreach (Instruction i in comparison.getAllFalsePositives())
                falsePositivesCombined.Items.Add(i);
        }

        private void ReloadCorrect()
        {
            correct.Items.Clear();
            foreach (Instruction i in comparison.getCorrect())
                correct.Items.Add(i);
        }
        /*
                private void ReloadMapped()
                {
                    mapped.Items.Clear();
                    foreach (Instruction i in comparison.getMapped())
                        mapped.Items.Add(i);
                }
        */
        private void swap_Click(object sender, EventArgs e)
        {
            this.Enabled = false;
            comparison.Swap();
            ReloadAll();
            this.Enabled = true;
        }

        private void ReloadAll()
        {
            ReloadLeft();
            ReloadRight();
            ReloadAllFalseNegatives();
            ReloadCorrect();
            //ReloadMapped();
            ReloadFalseNegatives();
            ReloadFalsePositives();
            ReloadOk();
            ReloadScore();
        }

        private void faulty_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (((Instruction)((ListBox)sender).SelectedItem) != null)
            {
                Program1.SetSelected(((Instruction)((ListBox)sender).SelectedItem).index, true);
            }
        }

        private void Program1_SelectedIndexChanged(object sender, EventArgs e)
        {
            ReloadFalsePositives();
            ReloadFalseNegatives();
            ReloadOk();
            Program2.Refresh();
            Program2.ClearSelected();
        }

        private void ReloadOk()
        {
            Ok.Items.Clear();
            if (currentInstructionProgram1 != null)
            {
                foreach (Instruction ins in currentInstructionProgram1.getOk())
                    Ok.Items.Add(ins);
            }
        }

        private void ReloadFalseNegatives()
        {
            falseNegatives.Items.Clear();
            if (currentInstructionProgram1 != null)
            {
                foreach (Instruction ins in currentInstructionProgram1.getFalseNegatives())
                    falseNegatives.Items.Add(ins);
            }
        }

        private void ReloadFalsePositives()
        {
            falsePositives.Items.Clear();
            if (currentInstructionProgram1 != null)
            {
                foreach (Instruction ins in currentInstructionProgram1.getFalsePositives())
                    falsePositives.Items.Add(ins);
            }
        }

        private void falsePositives_SelectedIndexChanged(object sender, EventArgs e)
        {
            int i = ((ListBox)sender).SelectedIndex;
            if (((Instruction)((ListBox)sender).SelectedItem) != null)
            {
                Program2.SetSelected(((Instruction)((ListBox)sender).SelectedItem).index, true);
            }
            if (i != -1)
            {
                falseNegatives.ClearSelected();
                Ok.ClearSelected();
            }
            //falsePositives.SelectedIndex = i;
            //falsePositives.SelectedIndex = i;
        }

        private void falseNegatives_SelectedIndexChanged(object sender, EventArgs e)
        {
            int i = ((ListBox)sender).SelectedIndex;
            if (((Instruction)((ListBox)sender).SelectedItem) != null)
            {
                Program2.SetSelected(((Instruction)((ListBox)sender).SelectedItem).index, true);
            }
            if (i != -1)
            {
                falsePositives.ClearSelected();
                Ok.ClearSelected();
            }
            //falseNegatives.SelectedIndex = i;
            //falseNegatives.SelectedIndex = i;
        }

        private void Ok_SelectedIndexChanged(object sender, EventArgs e)
        {
            int i = ((ListBox)sender).SelectedIndex;
            if (((Instruction)((ListBox)sender).SelectedItem) != null)
            {
                Program2.SetSelected(((Instruction)((ListBox)sender).SelectedItem).index, true);
            }
            if (i != -1)
            {
                falseNegatives.ClearSelected();
                falsePositives.ClearSelected();
            }
            //Ok.SelectedIndex = i;
            //Ok.SelectedIndex = i;
        }

        private void ReloadComparisons()
        {
            if (currentInstructionProgram1 != null && currentInstructionProgram2 != null)
            {
                Settings settings = ReadSettings();
                System.Console.WriteLine("----------------");
                System.Console.WriteLine("Basic Bock 1: 0x" + currentInstructionProgram1.basicBlock.from.ToString("x"));
                System.Console.WriteLine("Basic Bock 2: 0x" + currentInstructionProgram2.basicBlock.from.ToString("x"));
                System.Console.WriteLine("Order Comparison: " + comparison.CompareBblByOrder(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
                System.Console.WriteLine("Ins Comparison: " + comparison.CompareBblByInstructions(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
                System.Console.WriteLine("Cfg Comparison: (U) " + comparison.CompareBblByCfgBackward(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, settings.cfgLevel) + " (D) " + comparison.CompareBblByCfgForward(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, settings.cfgLevel) + " (B) " + comparison.CompareBblByCfg(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, Direction.BOTH, settings.cfgLevel));
                System.Console.WriteLine("Dfg Comparison: (U) " + comparison.CompareBblByDfgBackward(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, settings.dfgLevel) + " (D) " + comparison.CompareBblByDfgForward(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, settings.dfgLevel) + " (B) " + comparison.CompareBblByDfg(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, Direction.BOTH, settings.dfgLevel));
                System.Console.WriteLine("Frequency Comparison: " + comparison.CompareBblByFrequency(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
                System.Console.WriteLine("Syscall Comparison: " + comparison.CompareBblBySyscalls(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
                System.Console.WriteLine("Data Comparison: " + comparison.CompareBblByData(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
                this.Enabled = true;
                //System.Console.WriteLine("Dfg Comparison: " + comparison.getDfgScore(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
            }
        }

        private void button4_Click(object sender, EventArgs e)
        {
            comparison.DetermineCorrectMapping();
            Program1.Refresh();
            Program2.Refresh();
            ReloadAllFalseNegatives();
            ReloadCorrect();
            //            ReloadMapped();
        }
        /*
        private void openFileDialogMappingReference_FileOk(object sender, CancelEventArgs e)
        {
            textBoxMappingReference.Text = openFileDialogMappingReference.FileName;
            comparison.RegisterMapping1(textBoxMappingReference.Text);
            loadComparison.Enabled = true;
            loadMappingReference.Enabled = false;
        }
        private void openFileDialogMappingComparison_FileOk(object sender, CancelEventArgs e)
        {
            textBoxMappingComparison.Text = openFileDialogMappingComparison.FileName;
            comparison.RegisterMapping2(textBoxMappingComparison.Text);
            loadMappingComparison.Enabled = false;
            button3.Enabled = true;
            button4.Enabled = true;
            GuessMapping.Enabled = true;
        }*/
        private void GuessMapping_Click(object sender, EventArgs e)
        {
            if (comparison == null || comparison.program1 == null)
            {
                if (MessageBox.Show("No programs loaded yet, shall I load now?", "Error, no programs loaded", MessageBoxButtons.OKCancel) == DialogResult.Cancel)
                    return;
                else
                    comparison = new Comparison(program1TextBox.Text, program2TextBox.Text);
                //LoadPrograms(program1TextBox.Text, program2TextBox.Text);
            }
            this.Enabled = false;
            Settings settings = ReadSettings();

            if (settings.phase == Phase.INIT)
                AppliedSettingsListBox.Items.Clear();
            AppliedSettingsListBox.Items.Add(settings);

            comparison.DoMapping(settings);

            AfterGuess();
        }

        private void ReloadScore()
        {
            scoreOK.Text = "OK: " + comparison.NrOk + "   (" + ((Double)(100 * (comparison.NrOk * 1.0 / (comparison.NrOk + comparison.NrFn)))).ToString("###.##") + "%)";
            scoreFN.Text = "FN: " + comparison.NrFn + "   (" + ((Double)(100 * (comparison.NrFn * 1.0 / (comparison.NrOk + comparison.NrFn)))).ToString("###.##") + "%)";
            scoreFP.Text = "FP: " + comparison.NrFp + "   (" + ((Double)(100 * (comparison.NrFp * 1.0 / (comparison.NrOk + comparison.NrFn)))).ToString("###.##") + "%)"; ;
            scoreGOAL.Text = "Goal: " + (comparison.NrOk + comparison.NrFn);
        }

        private void loadProgram1_Click(object sender, EventArgs e)
        {
            DialogResult result = folderBrowserDialog1.ShowDialog();
            if (result == DialogResult.OK)
            {
                program1TextBox.Text = folderBrowserDialog1.SelectedPath;
            }
        }

        private void loadProgram2_Click(object sender, EventArgs e)
        {
            DialogResult result = folderBrowserDialog2.ShowDialog();
            if (result == DialogResult.OK)
            {
                program2TextBox.Text = folderBrowserDialog2.SelectedPath;
            }
        }

        private void load_Click(object sender, EventArgs e)
        {
            this.Enabled = false;
            comparison = new Comparison(program1TextBox.Text, program2TextBox.Text);
            ReloadLeft();
            Program1.Refresh();
            ReloadRight();
            ReloadAllFalseNegatives();
            ReloadCorrect();
            Program1.Refresh();
            Program2.Refresh();
            ReloadAllFalseNegatives();
            ReloadCorrect();
            ReloadScore();
            this.Enabled = true;
        }

        private void correct_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (((Instruction)((ListBox)sender).SelectedItem) != null)
            {
                Program1.SetSelected(((Instruction)((ListBox)sender).SelectedItem).index, true);
            }
        }

        private void mapped_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (((Instruction)((ListBox)sender).SelectedItem) != null)
            {
                Program1.SetSelected(((Instruction)((ListBox)sender).SelectedItem).index, true);
            }
        }

        private void consoleOutput_TextChanged(object sender, EventArgs e)
        {
        }

        private void Program2_SelectedIndexChanged(object sender, EventArgs e)
        {
            ReloadComparisons();
        }

        private void falsePositivesCombined_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (((Instruction)((ListBox)sender).SelectedItem) != null)
            {
                Program1.SetSelected(((Instruction)((ListBox)sender).SelectedItem).index, true);
            }
        }
        
        private void AfterGuess()
        {
            Program1.Refresh();
            ReloadAllFalseNegatives();
            ReloadAllFalsePositives();
            ReloadCorrect();
            //            ReloadMapped();
            ReloadScore();
            this.Enabled = true;
        }

        private Settings ReadSettings()
        {
            /*thresholds*/
            double cfgThreshold = Double.Parse(this.cfgThreshold.Text/*.Replace(".", ",")*/);
            double dfgThreshold = Double.Parse(this.dfgThreshold.Text/*.Replace(".", ",")*/);
            double insThreshold = Double.Parse(this.insThres.Text/*.Replace(".", ",")*/);
            double orderThreshold = Double.Parse(this.orderThres.Text/*.Replace(".", ",")*/);
            double execThreshold = Double.Parse(this.execThres.Text/*.Replace(".", ",")*/);
            double dataThreshold = Double.Parse(this.dataThres.Text/*.Replace(".", ",")*/);
            double syscallThreshold = Double.Parse(this.syscallThres.Text/*.Replace(".", ",")*/);

            /*minBblSize*/
            int minSizeBbl = Int32.Parse(this.minSizeBbl.Text);

            /*phase*/
            Phase phase = Phase.INIT;
            if (initial.Checked)
                phase = Phase.INIT;
            else if (filter.Checked)
                phase = Phase.FILTER;
            else if (extend.Checked)
                phase = Phase.EXTEND;

            /*when to match*/
            WhenToMatchOrUnmatch whenToMatch;
            if (matchBestOrUnmatchWorst.Checked)
                whenToMatch = WhenToMatchOrUnmatch.BESTORWORST;
            else if (matchFirst.Checked)
                whenToMatch = WhenToMatchOrUnmatch.FIRST;
            else
                whenToMatch = WhenToMatchOrUnmatch.ALL;

            /*only unmatched?*/
            bool only_unmatched;
            if (tryAll.Checked)
                only_unmatched = false;
            else
                only_unmatched = true;

            /*cfg and dfg parameters*/
            int dfgLevel = Int32.Parse(this.dfgLevel.Text);
            int cfgLevel = Int32.Parse(this.cfgLevel.Text);
            Direction cfDirection;
            Direction dfDirection;
            if (cfUp.Checked)
                cfDirection = Direction.UP;
            else if (cfDown.Checked)
                cfDirection = Direction.DOWN;
            else
                cfDirection = Direction.BOTH;
            if (dfUp.Checked)
                dfDirection = Direction.UP;
            else if (dfDown.Checked)
                dfDirection = Direction.DOWN;
            else
                dfDirection = Direction.BOTH;

            /*granularity*/
            Granularity granularity;
            if (BasicBlockLevel.Checked)
                granularity = Granularity.BASICBLOCK;
            else if (TransitionLevel.Checked)
                granularity = Granularity.TRANSITION;
            else granularity = Granularity.INSTRUCTION;

            return new Settings(granularity, phase, insThreshold, codeCheckBox.Checked, orderThreshold, orderCheckBox.Checked, execThreshold, frequencyCheckBox.Checked, dataThreshold, DataCheckBox.Checked, syscallThreshold, syscallCheckBox.Checked, cfgThreshold, cfgLevel, cfDirection, controlFlowCheckBox.Checked, dfgThreshold, dfgLevel, dfDirection, dataFlowCheckBox.Checked, only_unmatched, whenToMatch, minSizeBbl);
        }

        private void initial_CheckedChanged(object sender, EventArgs e)
        {
            if (initial.Checked)
            {
                dataFlowCheckBox.Checked = false;
                dataFlowCheckBox.Enabled = false;
                controlFlowCheckBox.Checked = false;
                controlFlowCheckBox.Enabled = false;
            }
            else
            {
                dataFlowCheckBox.Enabled = true;
                controlFlowCheckBox.Enabled = true;
            }
        }

        private void filter_CheckedChanged(object sender, EventArgs e)
        {
            if (filter.Checked)
            {
                matchBestOrUnmatchWorst.Text = "worst";
                minSizeBbl.Enabled = false;
                tryOnlyUnmapped.Checked = true;
                tryOnlyUnmapped.Text = "mapped";
                tryOnlyUnmapped.Enabled = false;
                tryAll.Enabled = false;
            }
            else
            {
                matchBestOrUnmatchWorst.Text = "best";
                minSizeBbl.Enabled = true;
                tryOnlyUnmapped.Text = "unmapped";
                tryOnlyUnmapped.Enabled = true;
                tryAll.Enabled = true;
            }
        }

        private void ApplySettings_Click(object sender, EventArgs e)
        {
            this.Enabled = false;
            foreach (Settings setting in toApplyListBox.CheckedItems)
            {
                comparison.DoMapping(setting);
                if (setting.phase == Phase.INIT)
                    AppliedSettingsListBox.Items.Clear();
                AppliedSettingsListBox.Items.Add(setting);
            }
            AppliedSettingsListBox.Refresh();
            AfterGuess();
        }

        private void copyToRight_Click(object sender, EventArgs e)
        {
            foreach (Settings setting in AppliedSettingsListBox.CheckedItems)
            {
                toApplyListBox.Items.Add(setting);
            }
            toApplyListBox.Refresh();
        }

        private void checkBox2_CheckedChanged(object sender, EventArgs e)
        {
            for (int i = 0; i < AppliedSettingsListBox.Items.Count; i++)
                AppliedSettingsListBox.SetItemChecked(i, appliedAllCheckbox.Checked);
        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            for (int i = 0; i < toApplyListBox.Items.Count; i++)
                toApplyListBox.SetItemChecked(i, toApplyCheckBoxAll.Checked);
        }

        private void SaveSettings_Click(object sender, EventArgs e)
        {
            List<Settings> toSave = new List<Settings>();
            saveFileDialog1.ShowDialog();
            if (!saveFileDialog1.CheckPathExists)
                return;

            foreach (Settings setting in AppliedSettingsListBox.Items)
            {
                toSave.Add(setting);
            }

            Settings.SaveListToFile(toSave, saveFileDialog1.FileName);
        }

        private void LoadSettings_Click(object sender, EventArgs e)
        {
            List<Settings> loaded;

            openFileDialog1.InitialDirectory = program1TextBox.Text;
            openFileDialog1.ShowDialog();
            if (!openFileDialog1.CheckFileExists)
                return;

            loaded = Settings.LoadListFromFile(openFileDialog1.FileName);

            foreach (Settings setting in loaded)
            {
                toApplyListBox.Items.Add(setting);
            }
        }

        private void RemoveSettingsFromListButton_Click(object sender, EventArgs e)
        {
            List<Settings> tmp = new List<Settings>();
            foreach (Settings setting in toApplyListBox.CheckedItems)
            {
                tmp.Add(setting);
            }
            foreach (Settings setting in tmp)
            {
                toApplyListBox.Items.Remove(setting);
            }
            toApplyListBox.Refresh();
        }

        private void loadSettingsButton_Click(object sender, EventArgs e)
        {
            if (toApplyListBox.SelectedItem != null)
                LoadSettings((Settings)toApplyListBox.SelectedItem);
        }

        private void MoveSettingsToListButton_Click(object sender, EventArgs e)
        {
            toApplyListBox.Items.Add(ReadSettings());
        }

    }
}