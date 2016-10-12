using System;
using System.Collections.Generic;
using System.Windows.Forms;
namespace EvaluateDiversity
{
    static class VisualMain
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            MainAndClassifiers form = new MainAndClassifiers();
            ConsoleForm console = new ConsoleForm();
            ConsoleToTextBox writer = new ConsoleToTextBox(form.inspection.consoleOutput, form.inspection);
            Console.SetOut(writer);
            Application.Run(form);
        }
    }
    class ConsoleToTextBox : System.IO.TextWriter
    {
        delegate void SetTextCallback(string text);
        internal ConsoleToTextBox(TextBox textBox, Form form)
            : base()
        {
            _textBox = textBox;
            _form = form;
        }
        private TextBox _textBox;
        private Form _form;
        public override System.Text.Encoding Encoding
        {
            get { return System.Text.Encoding.Default; }
        }
        // when the Console's Out property is set to other than the default,
        // the Console class will create a synchronized, thread-safe TextWriter
        // so we don't need to perform the otherwise required calls to
        // .InvokeRequired and .Invoke() on the TextBox.
        public override void Write(string value)
        {
            if (_textBox.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(Write);
                _form.Invoke(d, new object[] { value });
            }
            else
            {
                _textBox.AppendText(value.Replace("\n", base.NewLine));
            }
        }

        public override void WriteLine(string value)
        {
            this.Write(value + "\n");
        }
    }
}