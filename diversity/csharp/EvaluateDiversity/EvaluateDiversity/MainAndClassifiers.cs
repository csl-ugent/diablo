using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using Microsoft.Office.Interop.Excel;

namespace EvaluateDiversity
{
    public partial class MainAndClassifiers : Form
    {
        Comparison comparison;

        public ConsoleForm console;
        public InspectionForm inspection;

        public MainAndClassifiers()
        {
            console = new ConsoleForm();
            inspection = new InspectionForm();
            InitializeComponent();
            LoadSettings(Settings.Default);
        }

        private void DrawChart(List<Result> toDraw)
        {
            Microsoft.Office.Interop.Excel.Worksheet ws = ExcelStart();

            for (int i = 0; i < toDraw.Count; i++)
            {
                ws.Cells[1, i + 1] = toDraw[i].settings.ToString();
                ws.Cells[2, i + 1] = toDraw[i].OK;
                ws.Cells[3, i + 1] = toDraw[i].Fp;
            }

            ExcelDrawChart(toDraw.Count, ws);
        }

        public static Microsoft.Office.Interop.Excel.Worksheet ExcelStart()
        {
            Microsoft.Office.Interop.Excel.Application xla = new Microsoft.Office.Interop.Excel.Application();
            xla.Visible = true;
            Microsoft.Office.Interop.Excel.Workbook wb = xla.Workbooks.Add(Microsoft.Office.Interop.Excel.XlSheetType.xlWorksheet);
            Microsoft.Office.Interop.Excel.Worksheet ws = (Microsoft.Office.Interop.Excel.Worksheet)xla.ActiveSheet;
            return ws;
        }

        private static void ExcelDrawLineChart(Int32 row, Int32 column, Int32 rowCount, Int32 columnCount, Microsoft.Office.Interop.Excel.Worksheet ws)
        {
            ChartObjects chartObjs = (ChartObjects)ws.ChartObjects(Type.Missing);
            ChartObject chartObj = chartObjs.Add(100, 100, 400, 300);
            Chart xlChart = chartObj.Chart;
            Range chartRange = ws.get_Range(IndexToString(row, column), IndexToString(row + rowCount - 1, column + columnCount - 1));
            xlChart.SetSourceData(chartRange, Type.Missing);
            xlChart.ChartType = XlChartType.xlLine;
        }

        private static void ExcelDrawColumnChart(Int32 row, Int32 column, Int32 rowCount, Int32 columnCount, Microsoft.Office.Interop.Excel.Worksheet ws)
        {
            ChartObjects chartObjs = (ChartObjects)ws.ChartObjects(Type.Missing);
            ChartObject chartObj = chartObjs.Add(100, 100, 400, 300);
            Chart xlChart = chartObj.Chart;
            Range chartRange = ws.get_Range(IndexToString(row, column), IndexToString(row + rowCount - 1, column + columnCount - 1));
            xlChart.SetSourceData(chartRange, Type.Missing);
            xlChart.ChartType = XlChartType.xlColumnClustered;
        }

        private static void ExcelDrawChart(Int32 count, Microsoft.Office.Interop.Excel.Worksheet ws)
        {
            ChartObjects chartObjs = (ChartObjects)ws.ChartObjects(Type.Missing);
            ChartObject chartObj = chartObjs.Add(100, 100, 300, 300);
            Chart xlChart = chartObj.Chart;
            Object obj = ws.Cells[3, 10];
            Range chartRange = ws.get_Range("A2", ws.Cells[3, count]);
            xlChart.SetSourceData(chartRange, Type.Missing);
            xlChart.ChartType = XlChartType.xlLine;
        }

        private static void ExcelDraw3DAreaChart(Microsoft.Office.Interop.Excel.Worksheet ws, int startRow, int startColumn, int endRow, int endColumn)
        {
            ChartObjects chartObjs = (ChartObjects)ws.ChartObjects(Type.Missing);
            ChartObject chartObj = chartObjs.Add(100, 100, 300, 300);
            Chart xlChart = chartObj.Chart;
            Range chartRange = ws.get_Range(ws.Cells[startRow, startColumn], ws.Cells[endRow, endColumn]);
            xlChart.SetSourceData(chartRange, Type.Missing);
            xlChart.ChartType = XlChartType.xl3DArea;
        }

        private List<Result> Exhaustive(Settings settings)
        {
            if (comparison == null || comparison.program1 == null)
            {
                MessageBox.Show("No programs loaded!");
                return null;
            }

            List<Result> ret = new List<Result>();
            WhenToMatchOrUnmatch[] whenToMatches;
            if (TestWhichToAdd.Checked)
            {
                whenToMatches = new WhenToMatchOrUnmatch[2];
                whenToMatches[0] = WhenToMatchOrUnmatch.BESTORWORST;
                whenToMatches[1] = WhenToMatchOrUnmatch.ALL;
            }
            else
            {
                whenToMatches = new WhenToMatchOrUnmatch[1];
                whenToMatches[0] = settings.whenToMatch;
            }

            UInt32[] whichToConsiders = { settings.maxMatches };

            Double[] codeThresholds = MakeArrayOfThresholds(TestCodeThreshold.Checked, insThres, toIns, stepIns);

            Double[] dataThresholds = MakeArrayOfThresholds(TestDataThreshold.Checked, dataThres, toData, stepData);

            Double[] cfThresholds = MakeArrayOfThresholds(TestControlFlowThreshold.Checked, cfgThreshold, toCF, stepCF);

            Double[] dfThresholds = MakeArrayOfThresholds(TestDataFlowThreshold.Checked, dfgThreshold, toDF, stepDF);

            Double[] frequencyThresholds = MakeArrayOfThresholds(TestFrequencyThreshold.Checked, execThres, toFrequency, stepFrequency);

            Double[] orderThresholds = MakeArrayOfThresholds(TestOrderThreshold.Checked, orderThres, toOrder, stepOrder);

            Double[] syscallThresholds = MakeArrayOfThresholds(TestSyscallsThreshold.Checked, syscallThres, toSyscalls, stepSyscalls);

            Direction[] cfDirections = MakeArrayOfDirections(TestDirectionCF.Checked, settings.cfDirection);

            Direction[] dfDirections = MakeArrayOfDirections(TestDirectionDF.Checked, settings.dfDirection);

            Int32[] cfDistances = MakeArrayOfDistances(TestDistanceCF.Checked, cfgDistance, toDistanceCF, stepDistanceCF);

            Int32[] dfDistances = MakeArrayOfDistances(TestDistanceDF.Checked, dfgDistance, toDistanceDF, stepDistanceDF);

            Boolean[] codePareto = MakeArrayOfParetoBooleans(codeVaryPareto.Checked, codeCheckBox.CheckState == CheckState.Checked);
            Boolean[] dataPareto = MakeArrayOfParetoBooleans(dataVaryPareto.Checked, DataCheckBox.CheckState == CheckState.Checked);
            Boolean[] syscallsPareto = MakeArrayOfParetoBooleans(syscallsVaryPareto.Checked, syscallCheckBox.CheckState == CheckState.Checked);
            Boolean[] frequencyPareto = MakeArrayOfParetoBooleans(frequencyVaryPareto.Checked, frequencyCheckBox.CheckState == CheckState.Checked);
            Boolean[] orderPareto = MakeArrayOfParetoBooleans(orderVaryPareto.Checked, orderCheckBox.CheckState == CheckState.Checked);
            Boolean[] dfPareto = MakeArrayOfParetoBooleans(dfVaryPareto.Checked, dataFlowCheckBox.CheckState == CheckState.Checked);
            Boolean[] cfPareto = MakeArrayOfParetoBooleans(cfVaryPareto.Checked, controlFlowCheckBox.CheckState == CheckState.Checked);

            foreach (WhenToMatchOrUnmatch whenToMatch in whenToMatches)
            {
                settings.whenToMatch = whenToMatch;
                foreach (UInt32 whichToConsider in whichToConsiders)
                {
                    settings.maxMatches = whichToConsider;
                    foreach (Double a in codeThresholds)
                    {
                        settings.insThreshold = a;
                        foreach (Double b in dataThresholds)
                        {
                            settings.dataThreshold = b;
                            foreach (Double c in cfThresholds)
                            {
                                settings.cfgThreshold = c;
                                foreach (Double d in dfThresholds)
                                {
                                    settings.dfgThreshold = d;
                                    foreach (Double e in frequencyThresholds)
                                    {
                                        settings.execThreshold = e;
                                        foreach (Double f in orderThresholds)
                                        {
                                            settings.orderThreshold = f;
                                            foreach (Double g in syscallThresholds)
                                            {
                                                settings.syscallThreshold = g;
                                                foreach (Direction h in cfDirections)
                                                {
                                                    settings.cfDirection = h;
                                                    foreach (Direction i in dfDirections)
                                                    {
                                                        settings.dfDirection = i;
                                                        foreach (Int32 j in cfDistances)
                                                        {
                                                            settings.cfgLevel = j;
                                                            foreach (Int32 k in dfDistances)
                                                            {
                                                                settings.dfgLevel = k;
                                                                foreach (Boolean l in codePareto)
                                                                {
                                                                    settings.codePareto = l;
                                                                    foreach (Boolean m in dataPareto)
                                                                    {
                                                                        settings.dataPareto = m;
                                                                        foreach (Boolean n in syscallsPareto)
                                                                        {
                                                                            settings.syscallsPareto = n;

                                                                            foreach (Boolean o in orderPareto)
                                                                            {
                                                                                settings.orderPareto = o;
                                                                                foreach (Boolean p in frequencyPareto)
                                                                                {
                                                                                    settings.frequencyPareto = p;
                                                                                    foreach (Boolean q in cfPareto)
                                                                                    {
                                                                                        settings.controlFlowPareto = q;
                                                                                        foreach (Boolean r in dfPareto)
                                                                                        {
                                                                                            settings.dataFlowPareto = r;
                                                                                            comparison.DoMapping(settings);
                                                                                            ret.Add(new Result(new Settings(settings), ((Double)(100 * (comparison.NrOk * 1.0 / (comparison.NrOk + comparison.NrFn)))), ((Double)(100 * (comparison.NrFp * 1.0 / (comparison.NrOk + comparison.NrFn))))));
                                                                                            Console.WriteLine(settings.ToString() + " + SCORE:    OK: " + comparison.NrOk + "   (" + ((Double)(100 * (comparison.NrOk * 1.0 / (comparison.NrOk + comparison.NrFn)))).ToString("###.##") + "%)" + "       FN: " + comparison.NrFn + "   (" + ((Double)(100 * (comparison.NrFn * 1.0 / (comparison.NrOk + comparison.NrFn)))).ToString("###.##") + "%)" + "    FP: " + comparison.NrFp + "   (" + ((Double)(100 * (comparison.NrFp * 1.0 / (comparison.NrOk + comparison.NrFn)))).ToString("###.##") + "%)" + "    Goal: " + (comparison.NrOk + comparison.NrFn));
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return ret;
        }

        private bool[] MakeArrayOfParetoBooleans(bool p, bool passed)
        {
            bool[] ret;
            if (p)
            {
                ret = new bool[2];
                ret[0] = true;
                ret[1] = false;
            }
            else
            {
                ret = new bool[1];
                ret[0] = passed;
            }
            return ret;
        }

        private int[] MakeArrayOfDistances(bool p, System.Windows.Forms.TextBox fromT, System.Windows.Forms.TextBox toT, System.Windows.Forms.TextBox stepT)
        {
            int[] ret;
            if (p)
            {
                int from = Int32.Parse(fromT.Text);
                int to = Int32.Parse(toT.Text);
                int step = Int32.Parse(stepT.Text);
                ret = new Int32[((to - from) / step) + 1];
                int value;
                int i = 0;
                for (value = from; value <= to; value += step)
                {
                    ret[i++] = value;
                }
            }
            else
            {
                ret = new Int32[1];
                ret[0] = Int32.Parse(fromT.Text);
            }
            return ret;
        }

        private Direction[] MakeArrayOfDirections(Boolean testAll, Direction given)
        {
            Direction[] directions;
            if (testAll)
            {
                directions = new Direction[3];
                directions[0] = Direction.BOTH;
                directions[1] = Direction.DOWN;
                directions[2] = Direction.UP;
            }
            else
            {
                directions = new Direction[1];
                directions[0] = given;
            }
            return directions;
        }

        private double[] MakeArrayOfThresholds(bool p, System.Windows.Forms.TextBox fromT, System.Windows.Forms.TextBox toT, System.Windows.Forms.TextBox stepT)
        {
            Double[] ret;
            if (p)
            {
                Int32 from = (Int32)(Double.Parse(fromT.Text) * 100);
                Int32 to = (Int32)(Double.Parse(toT.Text) * 100);
                Int32 step = (Int32)(Double.Parse(stepT.Text) * 100);
                ret = new Double[(to - from) / step + 1];
                Double value;
                int i = 0;
                for (value = from; value <= to; value += step)
                {
                    ret[i++] = value * 1.0 / 100;
                }
            }
            else
            {
                ret = new Double[1];
                ret[0] = Double.Parse(fromT.Text);
            }
            return ret;
        }

        private void LoadSettings(Settings settings)
        {
            /*checkboxes*/
            codeCheckBox.CheckState = (!settings.code) ? CheckState.Unchecked : ((settings.codePareto) ? CheckState.Checked : CheckState.Indeterminate);
            frequencyCheckBox.CheckState = (!settings.frequency) ? CheckState.Unchecked : ((settings.frequencyPareto) ? CheckState.Checked : CheckState.Indeterminate);
            orderCheckBox.CheckState = (!settings.order) ? CheckState.Unchecked : ((settings.orderPareto) ? CheckState.Checked : CheckState.Indeterminate);
            dataFlowCheckBox.CheckState = (!settings.dataFlow) ? CheckState.Unchecked : ((settings.dataFlowPareto) ? CheckState.Checked : CheckState.Indeterminate);
            controlFlowCheckBox.CheckState = (!settings.controlFlow) ? CheckState.Unchecked : ((settings.controlFlowPareto) ? CheckState.Checked : CheckState.Indeterminate);
            DataCheckBox.CheckState = (!settings.data) ? CheckState.Unchecked : ((settings.dataPareto) ? CheckState.Checked : CheckState.Indeterminate);
            syscallCheckBox.CheckState = (!settings.syscalls) ? CheckState.Unchecked : ((settings.syscallsPareto) ? CheckState.Checked : CheckState.Indeterminate);

            /*thresholds*/
            insThres.Text = settings.insThreshold.ToString();
            orderThres.Text = settings.orderThreshold.ToString();
            execThres.Text = settings.execThreshold.ToString();
            cfgThreshold.Text = settings.cfgThreshold.ToString();
            dfgThreshold.Text = settings.dfgThreshold.ToString();
            dataThres.Text = settings.dataThreshold.ToString();
            syscallThres.Text = settings.syscallThreshold.ToString();

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
                throw new Exception("Unsupported");

            /*only unmapped?*/
            maxMatches.Text = settings.maxMatches.ToString();

            /*parameters cfg and dfg*/
            dfgDistance.Text = settings.dfgLevel.ToString();
            cfgDistance.Text = settings.cfgLevel.ToString();
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
            inspection.ReloadAll();
            ReloadScore();
        }









        private void button4_Click(object sender, EventArgs e)
        {
            comparison.DetermineCorrectMapping();
            inspection.ReloadAll();
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
                MessageBox.Show("No programs loaded!");
                return;
            }
            this.Enabled = false;
            Settings settings = ReadSettings();

            if (settings.phase == Phase.INIT && settings.granularity == Granularity.BASICBLOCK)
                AppliedSettingsListBox.Items.Clear();
            AppliedSettingsListBox.Items.Add(settings);

            comparison.DoMapping(settings);

            AfterGuess();
        }

        private void ReloadScore()
        {
            toolStripStatusLabel1.Text = "SCORE:    OK: " + comparison.NrOk + "   (" + (100 * comparison.oKRate).ToString("###.##") + "%)" + "       FN: " + comparison.NrFn + "   (" + (100 * comparison.fnRate).ToString("###.##") + "%)" + "    FP: " + comparison.NrFp + "   (" + (100 * comparison.fpRate).ToString("###.##") + "%)" + "    Goal: " + (comparison.NrOk + comparison.NrFn); ;
            //scoreOK.Text = "OK: " + comparison.NrOk + "   (" + ((Double)(100 * (comparison.NrOk * 1.0 / (comparison.NrOk + comparison.NrFn)))).ToString("###.##") + "%)";
            //scoreFN.Text = "FN: " + comparison.NrFn + "   (" + ((Double)(100 * (comparison.NrFn * 1.0 / (comparison.NrOk + comparison.NrFn)))).ToString("###.##") + "%)";
            //scoreFP.Text = "FP: " + comparison.NrFp + "   (" + ((Double)(100 * (comparison.NrFp * 1.0 / (comparison.NrOk + comparison.NrFn)))).ToString("###.##") + "%)"; ;
            //scoreGOAL.Text = "Goal: " + (comparison.NrOk + comparison.NrFn);
        }



        private void consoleOutput_TextChanged(object sender, EventArgs e)
        {
        }



        private void AfterGuess()
        {
            inspection.ReloadAll();
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
            else if (matchAllMatches.Checked)
                whenToMatch = WhenToMatchOrUnmatch.ALL;
            else throw new Exception();

            /*only unmatched?*/
            UInt32 maxNrOfMatches = UInt32.Parse(this.maxMatches.Text);

            /*cfg and dfg parameters*/
            int dfgLevel = Int32.Parse(this.dfgDistance.Text);
            int cfgLevel = Int32.Parse(this.cfgDistance.Text);
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

            return new Settings(granularity, phase, insThreshold, codeCheckBox.CheckState != CheckState.Unchecked, codeCheckBox.CheckState == CheckState.Checked, orderThreshold, orderCheckBox.CheckState != CheckState.Unchecked, orderCheckBox.CheckState == CheckState.Checked, execThreshold, frequencyCheckBox.CheckState != CheckState.Unchecked, frequencyCheckBox.CheckState == CheckState.Checked, dataThreshold, DataCheckBox.CheckState != CheckState.Unchecked, DataCheckBox.CheckState == CheckState.Checked, syscallThreshold, syscallCheckBox.CheckState != CheckState.Unchecked, syscallCheckBox.CheckState == CheckState.Checked, cfgThreshold, cfgLevel, cfDirection, controlFlowCheckBox.CheckState != CheckState.Unchecked, controlFlowCheckBox.CheckState == CheckState.Checked, dfgThreshold, dfgLevel, dfDirection, dataFlowCheckBox.CheckState != CheckState.Unchecked, dataFlowCheckBox.CheckState == CheckState.Checked, maxNrOfMatches, whenToMatch);
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
                maxMatches.Enabled = false;
            }
            else
            {
                matchBestOrUnmatchWorst.Text = "best";
                maxMatches.Enabled = true;
            }
        }

        private void ApplySettings_Click(object sender, EventArgs e)
        {
            if (comparison == null || comparison.program1 == null)
            {
                MessageBox.Show("No programs loaded!");
                return;
            }
            this.Enabled = false;
            foreach (Settings setting in toApplyListBox.CheckedItems)
            {
                comparison.DoMapping(setting);
                if (setting.phase == Phase.INIT)
                    AppliedSettingsListBox.Items.Clear();
                AppliedSettingsListBox.Items.Add(setting);
                AppliedSettingsListBox.Refresh();
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

        private void consoleToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!console.Visible)
            {
                console.Visible = true;
                consoleToolStripMenuItem.Checked = true;
            }
            else
            {
                console.Visible = false;
                consoleToolStripMenuItem.Checked = false;
            }
        }

        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DialogResult result = folderBrowserDialog1.ShowDialog();
            if (result == DialogResult.OK)
            {
                loadFromPath(folderBrowserDialog1.SelectedPath);
            }
        }

        private void loadFromPath(String path)
        {
            this.Enabled = false;
            comparison = new Comparison(path + @"\version1\", path + @"\version2\");
            inspection.SetComparison(comparison);
            inspection.ReloadAll();
            ReloadScore();
            this.Enabled = true;
        }

        private void inspectionToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!inspection.Visible)
            {
                inspection.Visible = true;
                inspectionToolStripMenuItem.Checked = true;
            }
            else
            {
                inspection.Visible = false;
                inspectionToolStripMenuItem.Checked = false;
            }
        }

        private void LoadAppliedSettings_Click(object sender, EventArgs e)
        {
            if (AppliedSettingsListBox.SelectedItem != null)
                LoadSettings((Settings)AppliedSettingsListBox.SelectedItem);
        }

        private void advancedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!advancedToolStripMenuItem.Checked)
            {
                ToApplyBox.Visible = true;
                AppliedBox.Visible = true;
                TableAppliedButtons.Visible = true;
                TableToApplyButtons.Visible = true;
                ApplySettings.Visible = true;
                copyToRight.Visible = true;
                ApplySimple.Visible = false;
                this.Height = Math.Max(525, this.Height);
                advancedToolStripMenuItem.Checked = true;
            }
            else
            {
                ToApplyBox.Visible = false;
                AppliedBox.Visible = false;
                TableAppliedButtons.Visible = false;
                TableToApplyButtons.Visible = false;
                ApplySettings.Visible = false;
                copyToRight.Visible = false;
                ApplySimple.Visible = true;
                this.Height = 335;
                advancedToolStripMenuItem.Checked = false;
            }

        }

        private void viewToolStripMenuItem_Click(object sender, EventArgs e)
        {
            inspectionToolStripMenuItem.Checked = inspection.Visible;
            consoleToolStripMenuItem.Checked = console.Visible;
        }

        private void stepper(object sender, EventArgs e)
        {
            List<Result> results = Exhaustive(ReadSettings());
            if (results != null)
                DrawChart(results);
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.Dispose();
        }

        private void stepperToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (stepperToolStripMenuItem.Checked)
            {
                panelLabelDistance.Visible = false;
                PanelLabelThreshold.Visible = false;
                panelParametersDirectionAndButton.Visible = false;
                panelParametersDistance.Visible = false;
                panelParametersGlobalSettings.Visible = false;
                panelparametersThreshold.Visible = false;
                stepperToolStripMenuItem.Checked = false;
                button1.Visible = false;
            }
            else
            {
                panelLabelDistance.Visible = true;
                PanelLabelThreshold.Visible = true;
                panelParametersDirectionAndButton.Visible = true;
                panelParametersDistance.Visible = true;
                panelParametersGlobalSettings.Visible = true;
                panelparametersThreshold.Visible = true;
                stepperToolStripMenuItem.Checked = true;
                button1.Visible = true;
            }
        }

        private void instructionselectionvsinstructionsyntax(object sender, EventArgs e)
        {
            Int32 stepsize = 50;
            Microsoft.Office.Interop.Excel.Worksheet ws = EvaluateDiversity.MainAndClassifiers.ExcelStart();


            for (int i = 0; i <= 100; i += stepsize)
            {
                ws.Cells[i / stepsize + 2, 1] = i.ToString();
                ws.Cells[100 / stepsize + 2 + i / stepsize + 2, 1] = i.ToString();
                for (int j = 0; j <= 100; j++)
                {
                    ws.Cells[1, j + 2] = j * 1.0 / 100;
                    ws.Cells[100 / stepsize + 2 + 1, j + 2] = j * 1.0 / 100;
                }

                string dir1 = @"U:\benchmarks\dr\IS\chance" + i + @"\version1\";
                string dir2 = @"U:\benchmarks\dr\IS\chance" + i + @"\version2\";

                EvaluateDiversity.Comparison comparison = new EvaluateDiversity.Comparison(dir1, dir2);

                comparison.FakeMatches();

                Int64[] result = comparison.GetScoresBasedOnInstructions(EvaluateDiversity.ClassifierType.CONTROLFLOW);

                Int64 totalNrOfValidMatches = 0;
                for (int j = 0; j <= 100; j++)
                {
                    totalNrOfValidMatches += result[j];
                }

                Int64 nrFnLeft = 0;
                //TODO: total number of instructions
                Int64 maximumNumberOfFalsePositives = comparison.program1.Count * comparison.program2.Count - totalNrOfValidMatches;
                Int64 nrFpLeft = maximumNumberOfFalsePositives;

                ws.Cells[1, 104] = totalNrOfValidMatches;
                ws.Cells[100 / stepsize + 3, 104] = maximumNumberOfFalsePositives;

                for (int j = 0; j <= 100; j++)
                {
                    ws.Cells[i / stepsize + 2, j + 2] = nrFnLeft * 1.0 / totalNrOfValidMatches;
                    ws.Cells[i / stepsize + 2, 102 + j + 2] = nrFnLeft;
                    nrFnLeft += result[j];

                    ws.Cells[i / stepsize + 100 / stepsize + 2 + 2, j + 2] = nrFpLeft * 1.0 / (nrFpLeft + totalNrOfValidMatches - nrFpLeft);
                    ws.Cells[i / stepsize + 100 / stepsize + 2 + 2, 102 + j + 2] = nrFpLeft;
                    nrFpLeft -= result[j + 101];
                }
            }

            ExcelDraw3DAreaChart(ws, 1, 1, 100 / stepsize + 2, 102);
        }


        // Converts (row,col) indices to an Excel-style A1:C1 string
        public static String IndexToString(int row, int col)
        {
            string result = "";

            if (col > 26)
            {
                result += (char)('A' + (col - 1) / 26 - 1);
                result += (char)('A' + (col - 1) % 26);
                result += row;
            }
            else
            {
                result += (char)('A' + (col - 1) % 26);
                result += row;
            }

            return result;
        }

        private void CollectFalsePositiveRates(object sender, EventArgs e)
        {
            string dir1 = @"U:\benchmarks\experiment1\";
            string dir2 = @"U:\benchmarks\experiment1\";
            comparison = new EvaluateDiversity.Comparison(dir1, dir2);
            comparison.FakeMatches();
            inspection.SetComparison(comparison);
            inspection.ReloadAll();

            Int64[] result;
            foreach (ClassifierType t in Enum.GetValues(typeof(ClassifierType)))
            {
                result = comparison.GetScoresBasedOnBasicBlocks(t);
                TurnResultsIntoExcel(result, "bbl" + t.ToString() + "Rate.xlsx");
            }

            foreach (ClassifierType t in Enum.GetValues(typeof(ClassifierType)))
            {
                result = comparison.GetScoresBasedOnInstructions(t);
                TurnResultsIntoExcel(result, "ins" + t.ToString() + "Rate.xlsx");
            }
        }

        private void TurnResultsIntoExcel(Int64[] result, string name)
        {
            Microsoft.Office.Interop.Excel.Worksheet ws = EvaluateDiversity.MainAndClassifiers.ExcelStart();
            for (int j = 0; j <= 100; j++)
            {
                ws.Cells[1, j + 1] = result[j];
                ws.Cells[2, j + 1] = result[j + 101];
            }

            ws.Cells[4, 100 + 1] = "=" + IndexToString(1, 100 + 1);
            ws.Cells[5, 100 + 1] = "=" + IndexToString(2, 100 + 1);

            ws.Cells[7, 100 + 1] = "=" + IndexToString(5, 100 + 1) + "/(" + IndexToString(5, 100 + 1) + "+" + IndexToString(4, 100 + 1) + ")";
            ws.Cells[8, 100 + 1] = "=" + IndexToString(5, 100 + 1) + "/$A$5";

            for (int j = 99; j >= 0; j--)
            {
                ws.Cells[4, j + 1] = "=" + IndexToString(1, j + 1) + "+" + IndexToString(4, j + 2) + "";
                ws.Cells[5, j + 1] = "=" + IndexToString(2, j + 1) + "+" + IndexToString(5, j + 2) + "";

                ws.Cells[7, j + 1] = "=" + IndexToString(5, j + 1) + "/(" + IndexToString(5, j + 1) + "+" + IndexToString(4, j + 1) + ")";
                ws.Cells[8, j + 1] = "=" + IndexToString(5, j + 1) + "/$A$5";
            }
            ExcelDrawLineChart(7, 1, 2, 101, ws);
            ws.SaveAs(name, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing);
        }

        private void collectSizesOfEquivalenceClasses(object sender, EventArgs e)
        {
            string dir1 = @"U:\benchmarks\experiment1\";
            string dir2 = @"U:\benchmarks\experiment1\";
            comparison = new EvaluateDiversity.Comparison(dir1, dir2);
            comparison.FakeMatches();
            inspection.SetComparison(comparison);
            inspection.ReloadAll();

            foreach (ClassifierType t in Enum.GetValues(typeof(ClassifierType)))
            {
                Dictionary<Int32, List<BasicBlock>> resultBbl = comparison.GetClassCountsBasedOnBasicBlocks(t);
                TurnResultsIntoExcel2(ToListOfIntegers2(resultBbl), "bbl" + t.ToString() + "FreqChart.xlsx");
            }

            foreach (ClassifierType t in Enum.GetValues(typeof(ClassifierType)))
            {
                Dictionary<Int32, List<Instruction>> resultIns = comparison.GetClassCountsBasedOnInstructions(t);
                TurnResultsIntoExcel2(ToListOfIntegers(resultIns), "ins" + t.ToString() + "FreqChart.xlsx");
            }
        }

        private void TurnResultsIntoExcel2(Int32[] list, string name)
        {
            Microsoft.Office.Interop.Excel.Worksheet ws = EvaluateDiversity.MainAndClassifiers.ExcelStart();
            int counter = 2;
            ws.Cells[2, 1] = "Title";
            ws.Cells[3, 1] = "Title2";
            for (int j = list.Length - 1; j >= 0; j--)
                if (list[j] != 0)
                {
                    ws.Cells[1, counter] = j;
                    ws.Cells[2, counter] = list[j] * j;
                    ws.Cells[3, counter] = list[j];
                    counter++;
                }
            ExcelDrawColumnChart(1, 1, 2, counter - 1, ws);
            ws.SaveAs(name, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing);

        }
        private static Int32[] ToListOfIntegers2(Dictionary<Int32, List<BasicBlock>> result)
        {
            int max = 0;
            foreach (Int32 i in result.Keys)
            {
                max = Math.Max(max, i);
            }
            Int32[] list = new Int32[max + 1];
            for (int i = 0; i < max + 1; i++)
            {
                if (result.ContainsKey(i))
                    list[i] = result[i].Count;
                else
                    list[i] = 0;
            }
            return list;
        }
        private static Int32[] ToListOfIntegers(Dictionary<Int32, List<Instruction>> result)
        {
            int max = 0;
            foreach (Int32 i in result.Keys)
            {
                max = Math.Max(max, i);
            }
            Int32[] list = new Int32[max + 1];
            for (int i = 0; i < max + 1; i++)
            {
                if (result.ContainsKey(i))
                    list[i] = result[i].Count;
                else
                    list[i] = 0;
            }
            return list;
        }

        private void collectFalsePositiveRatesForSyntaxAndDataCombined(object sender, EventArgs e)
        {
            string dir1 = @"U:\benchmarks\experiment1\";
            string dir2 = @"U:\benchmarks\experiment1\";
            comparison = new EvaluateDiversity.Comparison(dir1, dir2);
            comparison.FakeMatches();
            inspection.SetComparison(comparison);
            inspection.ReloadAll();

            Int64[,] result;
            result = comparison.GetScoresForInstructionSyntaxAndData();
            TurnResultsIntoExcel3(result, "threeDrules");
        }

        private void TurnResultsIntoExcel3(Int64[,] list, string name)
        {
            Microsoft.Office.Interop.Excel.Worksheet ws = EvaluateDiversity.MainAndClassifiers.ExcelStart();
            for (int i = 0; i < list.GetLength(0); i++)
                for (int j = 0; j < list.GetLength(1); j++)
                {
                    ws.Cells[i + 1, j + 1] = list[i, j];
                }
            ws.SaveAs(name, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing);
        }

        private void TurnResultsIntoExcel4(Double[,] list, string name)
        {
            Microsoft.Office.Interop.Excel.Worksheet ws = EvaluateDiversity.MainAndClassifiers.ExcelStart();
            ws.Cells[2, 1] = "fn";
            ws.Cells[2, 2] = 1;
            ws.Cells[3, 1] = "fp";
            ws.Cells[3, 2] = 0;
            ws.Cells[1, 2] = 0;
            for (int i = 0; i < list.GetLength(0); i++)
            {
                ws.Cells[1, i + 3] = i + 1;
                for (int j = 0; j < list.GetLength(1); j++)
                {
                    ws.Cells[j + 2, i + 3] = list[i, j];
                }
            }
            //ws.SaveAs(name, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing, System.Type.Missing);
        }

        private void ApplySettingsAndReport_Click(object sender, EventArgs e)
        {
            if (comparison == null || comparison.program1 == null)
            {
                MessageBox.Show("No programs loaded!");
                return;
            }
            this.Enabled = false;

            Double[,] result = new Double[toApplyListBox.CheckedItems.Count, 2];
            int i = 0;
            foreach (Settings setting in toApplyListBox.CheckedItems)
            {
                comparison.DoMapping(setting);
                result[i, 0] = comparison.fnRate;
                result[i++, 1] = comparison.fpRate;

                if (setting.phase == Phase.INIT)
                    AppliedSettingsListBox.Items.Clear();
                AppliedSettingsListBox.Items.Add(setting);
                AppliedSettingsListBox.Refresh();
            }
            AppliedSettingsListBox.Refresh();
            AfterGuess();
            TurnResultsIntoExcel4(result, "tmp");
        }

        private void button7_Click(object sender, EventArgs e)
        {
            Microsoft.Office.Interop.Excel.Worksheet ws = EvaluateDiversity.MainAndClassifiers.ExcelStart();
            int teller = 0;
            for (int i =20; i >= 1; i--)
                for (int j = i+1; j <= 50; j++)
                {
                    try
                    {

                        comparison = new Comparison(@"U:\benchmarks\experiment20\version" + i + @"\test\", @"U:\benchmarks\experiment20\version" + j + @"\test\");
                    }
                    catch (Exception)
                    {
                        ws.Cells[teller * 2 + 1, 1] = "fn: i=" + i + "j=" + j;
                        ws.Cells[teller * 2 + 2, 1] = "fp: i=" + i + "j=" + j;
                        teller++;
                        continue;
                    }
                    Double[,] result = new Double[toApplyListBox.CheckedItems.Count, 2];
                    int k = 0;
                    foreach (Settings setting in toApplyListBox.CheckedItems)
                    {
                        comparison.DoMapping(setting);
                        result[k, 0] = comparison.fnRate;
                        result[k++, 1] = comparison.fpRate;
                    }
                    
                    ws.Cells[teller*2+1, 1] = "fn: i="+i+"j="+ j;
                    ws.Cells[teller*2+1, 2] = 1;
                    ws.Cells[teller*2+2, 1] = "fp: i="+i+"j="+ j;
                    ws.Cells[teller*2+2, 2] = 0;
                    for (int l = 0; l < result.GetLength(0); l++)
                    {
                        for (int m = 0; m < result.GetLength(1); m++)
                        {
                            ws.Cells[teller*2+ m + 1, l + 3] = result[l, m];
                        }
                    }

                    teller++;
                }


        }

    }
}