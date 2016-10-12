using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace EvaluateDiversity
{
    public partial class InspectionForm : Form
    {
        private Comparison comparison;

        private Instruction currentInstructionProgram1
        {
            get { return (Instruction)Program1.SelectedItem; }
        }
        private Instruction currentInstructionProgram2
        {
            get { return (Instruction)Program2.SelectedItem; }
        }

        public InspectionForm()
        {
            InitializeComponent();

        }

        public void SetComparison(Comparison comparison)
        {
            this.comparison = comparison;
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

        private void ReloadLeft()
        {
            // Clear the items in the ListBox
            Program1.Items.Clear();
            int i = 0;
            // Re-add all items
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

        private void ReloadAllFalseNegatives()
        {
            falseNegativesCombined.Items.Clear();
            //TODO - Waarom?
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

        private void ReloadAllOk()
        {
            OkCombined.Items.Clear();
            foreach (Instruction i in comparison.getCorrect())
                OkCombined.Items.Add(i);
        }

        public void ReloadAll()
        {
            if (Visible && comparison!=null)
            {
                ReloadLeft();
                ReloadRight();
                ReloadAllFalseNegatives();
                ReloadAllFalsePositives();
                ReloadAllOk();
                ReloadFalseNegatives();
                ReloadFalsePositives();
                ReloadOk();
            }
        }

        private void Clear()
        {
            Program1.Items.Clear();
            Program2.Items.Clear();

            Ok.Items.Clear();
            falseNegatives.Items.Clear();
            falsePositives.Items.Clear();

            OkCombined.Items.Clear();
            falseNegativesCombined.Items.Clear();
            falsePositivesCombined.Items.Clear();
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

        private void ReloadComparisons()
        {
            if (currentInstructionProgram1 != null && currentInstructionProgram2 != null)
            {
                // TODO: Check the settings here! This causes the application to crash!
                Settings settings = Settings.Default;// ReadSettings();
                System.Console.WriteLine("\n----Basic Block----\n");
                System.Console.WriteLine("Basic Bock 1: 0x" + currentInstructionProgram1.basicBlock.from.ToString("x"));
                System.Console.WriteLine("Basic Bock 2: 0x" + currentInstructionProgram2.basicBlock.from.ToString("x"));
                System.Console.WriteLine("Order Comparison: " + comparison.CompareBblByOrder(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
                System.Console.WriteLine("Ins Comparison: " + comparison.CompareBblBySyntax(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
                System.Console.WriteLine("Cfg Comparison: (U) " + comparison.CompareBblByCfgBackward(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, settings.cfgLevel) + " (D) " + comparison.CompareBblByCfgForward(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, settings.cfgLevel) + " (B) " + comparison.CompareBblByControlFlow(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, Direction.BOTH, settings.cfgLevel));
                System.Console.WriteLine("Dfg Comparison: (U) " + comparison.CompareBblByDfgBackwardOrMinusOne(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, settings.dfgLevel) + " (D) " + comparison.CompareBblByDfgForwardOrMinusOne(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, settings.dfgLevel) + " (B) " + comparison.CompareBblByDataFlowOrMinusOne(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock, Direction.BOTH, settings.dfgLevel));
                System.Console.WriteLine("Frequency Comparison: " + comparison.CompareBblByFrequency(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
                System.Console.WriteLine("Syscall Comparison: " + comparison.CompareBblBySyscallsOrMinusOne(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
                System.Console.WriteLine("Data Comparison: " + comparison.CompareBblByDataOrMinusOne(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));

                System.Console.WriteLine("\n----Instruction----\n");
                System.Console.WriteLine("Instruction 1: 0x" + currentInstructionProgram1.address.ToString("x"));
                System.Console.WriteLine("Instruction 2: 0x" + currentInstructionProgram2.address.ToString("x"));
                System.Console.WriteLine("Order Comparison: " + comparison.CompareInsByOrder(currentInstructionProgram1, currentInstructionProgram2));
                System.Console.WriteLine("Ins Comparison: " + comparison.CompareInsBySyntax(currentInstructionProgram1, currentInstructionProgram2));
                System.Console.WriteLine("Cfg Comparison: (U) " + comparison.CompareInsByCfgBackward(currentInstructionProgram1, currentInstructionProgram2, settings.cfgLevel) + " (D) " + comparison.CompareInsByCfgForward(currentInstructionProgram1, currentInstructionProgram2, settings.cfgLevel) + " (B) " + comparison.CompareInsByControlFlow(currentInstructionProgram1, currentInstructionProgram2, Direction.BOTH, settings.cfgLevel));
                System.Console.WriteLine("Dfg Comparison: (U) " + comparison.CompareInsByDfgBackwardOrMinusOne(currentInstructionProgram1, currentInstructionProgram2, settings.dfgLevel) + " (D) " + comparison.CompareInsByDfgForwardOrMinusOne(currentInstructionProgram1, currentInstructionProgram2, settings.dfgLevel) + " (B) " + comparison.CompareInsByDataFlowOrMinusOne(currentInstructionProgram1, currentInstructionProgram2, Direction.BOTH, settings.dfgLevel));
                System.Console.WriteLine("Frequency Comparison: " + comparison.CompareInsByFrequency(currentInstructionProgram1, currentInstructionProgram2));
                System.Console.WriteLine("Syscall Comparison: " + comparison.CompareInsBySyscallsOrMinusOne(currentInstructionProgram1, currentInstructionProgram2));
                System.Console.WriteLine("Data Comparison: " + comparison.CompareInsByDataOrMinusOne(currentInstructionProgram1, currentInstructionProgram2));
                this.Enabled = true;
                //System.Console.WriteLine("Dfg Comparison: " + comparison.getDfgScore(currentInstructionProgram1.basicBlock, currentInstructionProgram2.basicBlock));
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

        private void Program2_SelectedIndexChanged(object sender, EventArgs e)
        {
            ReloadComparisons();
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
        
        private void correct_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (((Instruction)((ListBox)sender).SelectedItem) != null)
            {
                Program1.SetSelected(((Instruction)((ListBox)sender).SelectedItem).index, true);
            }
        }

        private void falsePositivesCombined_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (((Instruction)((ListBox)sender).SelectedItem) != null)
            {
                Program1.SetSelected(((Instruction)((ListBox)sender).SelectedItem).index, true);
            }
        }

        private void falseNegativesCombined_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (((Instruction)((ListBox)sender).SelectedItem) != null)
            {
                Program1.SetSelected(((Instruction)((ListBox)sender).SelectedItem).index, true);
            }
        }

        private void InspectionForm_VisibleChanged(object sender, EventArgs e)
        {
            if (this.Visible)
                ReloadAll();
            if (!this.Visible)
                Clear();
        }

        private void InspectionForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            this.Visible = false;
            e.Cancel = true;
        }
    }
}