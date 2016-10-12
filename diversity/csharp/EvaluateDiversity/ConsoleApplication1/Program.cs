using System;
using System.Collections.Generic;
using System.Text;

namespace ConsoleApplication1
{
    class Program
    {
        static void Main(string[] args)
        {
            //Microsoft.Office.Interop.Excel.Worksheet ws = EvaluateDiversity.MainAndClassifiers.ExcelStart();

            for (int i = 0; i < 100; i+=10)
            {
                string dir1 = @"U:\benchmarks\dr\IS\chance" + i + @"\version1\";
                string dir2 = @"U:\benchmarks\dr\IS\chance" + i + @"\version2\";
                EvaluateDiversity.Comparison comparison = new EvaluateDiversity.Comparison(dir1, dir2);
                Int64[] result = comparison.GetScoresBasedOnInstructions(EvaluateDiversity.ClassifierType.SYNTAX);
                
            }
            System.Console.ReadLine();
        }
    }
}
