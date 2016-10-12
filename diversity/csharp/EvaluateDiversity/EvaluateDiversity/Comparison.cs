using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

// TODO: replace all parses of reader.foo HEX -> INT with readHex(reader, argument)

namespace EvaluateDiversity
{
    public class Result
    {
        public Settings settings;
        public Double Fp;
        public Double OK;
        public Result(Settings settings, Double OK, Double Fp)
        {
            this.settings = settings;
            this.OK = OK;
            this.Fp = Fp;
        }
    }

    public enum ClassifierType
    {
        SYNTAX,
        DATA,
        CONTROLFLOW,
        DATAFLOW,
        FREQUENCY,
        ORDER,
        SYSCALLS
    }

    public enum ThreeState
    {
        TRUE,
        FALSE,
        SOSO,
    }
    public enum WhenToMatchOrUnmatch
    {
        ALL,
        BESTORWORST
    }

    public enum Direction
    {
        UP,
        DOWN,
        BOTH
    }
    public enum Phase
    {
        INIT,
        EXTEND,
        FILTER
    }
    public enum Granularity
    {
        BASICBLOCK,
        TRANSITION,
        INSTRUCTION
    }

    [Serializable]
    public class Settings
    {
        public Granularity granularity;
        public Phase phase;
        public Double insThreshold;
        public Double orderThreshold;
        public Double execThreshold;
        public int cfgLevel;
        public int dfgLevel;
        public Double cfgThreshold;
        public Double dfgThreshold;
        public Double dataThreshold;
        public Double syscallThreshold;
        public WhenToMatchOrUnmatch whenToMatch;
        public UInt32 maxMatches;
        public Direction cfDirection;
        public Direction dfDirection;

        public bool code;
        public bool data;
        public bool syscalls;
        public bool dataFlow;
        public bool controlFlow;
        public bool frequency;
        public bool order;

        public bool codePareto;
        public bool dataPareto;
        public bool syscallsPareto;
        public bool dataFlowPareto;
        public bool controlFlowPareto;
        public bool frequencyPareto;
        public bool orderPareto;

        public override string ToString()
        {
            String s = phase.ToString();
            s += " " + granularity;

            if (code)
            {
                s += " Instruction syntax: " + codePareto.ToString() + " " + insThreshold;
            }
            if (data)
            {
                s += " Data Values: " + dataPareto.ToString() + " " + dataThreshold;
            }
            if (syscalls)
            {
                s += " Syscalls: " + syscallsPareto.ToString() + " " + syscallThreshold;
            }
            if (dataFlow)
            {
                s += " DFG: " + dataFlowPareto.ToString() + " " + dfgThreshold + " distance " + dfgLevel + " direction " + dfDirection;
            }
            if (order)
            {
                s += " First time: " + orderPareto.ToString() + " " + orderThreshold;
            }
            if (controlFlow)
            {
                s += " CFG: " + controlFlowPareto.ToString() + " " + cfgThreshold + " distance " + cfgLevel + " direction " + cfDirection;
            }
            if (frequency)
            {
                s += " Frequency: " + frequencyPareto.ToString() + " " + execThreshold;
            }
            s += " " + whenToMatch + " ";
            s += " Max. matches? " + maxMatches;
            return s;
        }

        public static Settings Default = new Settings(Granularity.BASICBLOCK, 
                                                      Phase.INIT, 
                                                      0.7, 
                                                      true, 
                                                      true, 
                                                      0.1, 
                                                      false, 
                                                      false, 
                                                      0.7, 
                                                      true, 
                                                      true, 
                                                      0.5, 
                                                      true, 
                                                      true, 
                                                      0.3, 
                                                      true, 
                                                      true, 
                                                      0.1, 
                                                      5, 
                                                      Direction.BOTH, 
                                                      false, 
                                                      true, 
                                                      0.5, 
                                                      5, 
                                                      Direction.BOTH, 
                                                      false, 
                                                      true, 
                                                      1, 
                                                      WhenToMatchOrUnmatch.BESTORWORST);

        public Settings(
            Granularity granularity,
            Phase phase,
            Double insThreshold, bool code, bool codePareto,
            Double orderThreshold, bool order, bool orderPareto,
            Double execThreshold, bool frequency, bool frequencyPareto,
            Double dataThreshold, bool data, bool dataPareto,
            Double syscallThreshold, bool syscalls, bool syscallsPareto,
            Double cfgThreshold, int cfgLevel, Direction cfDirection, bool controlFlow, bool controlFlowPareto,
            Double dfgThreshold, int dfgLevel, Direction dfDirection, bool dataFlow, bool dataFlowPareto,
            UInt32 maxMatched,
            WhenToMatchOrUnmatch whenToMatch
            )
        {
            this.granularity = granularity;
            this.phase = phase;
            this.insThreshold = insThreshold;
            this.orderThreshold = orderThreshold;
            this.execThreshold = execThreshold;
            this.dataThreshold = dataThreshold;
            this.syscallThreshold = syscallThreshold;
            this.cfgThreshold = cfgThreshold;
            this.cfgLevel = cfgLevel;
            this.cfDirection = cfDirection;
            this.dfgThreshold = dfgThreshold;
            this.dfgLevel = dfgLevel;
            this.dfDirection = dfDirection;
            this.maxMatches = maxMatched;
            this.whenToMatch = whenToMatch;
            this.code = code;
            this.data = data;
            this.syscalls = syscalls;
            this.dataFlow = dataFlow;
            this.controlFlow = controlFlow;
            this.frequency = frequency;
            this.order = order;
            this.codePareto = codePareto;
            this.dataPareto = dataPareto;
            this.syscallsPareto = syscallsPareto;
            this.dataFlowPareto = dataFlowPareto;
            this.controlFlowPareto = controlFlowPareto;
            this.frequencyPareto = frequencyPareto;
            this.orderPareto = orderPareto;
        }

        public Settings(Settings from)
        {
            this.granularity = from.granularity;
            this.phase = from.phase;
            this.insThreshold = from.insThreshold;
            this.orderThreshold = from.orderThreshold;
            this.execThreshold = from.execThreshold;
            this.dataThreshold = from.dataThreshold;
            this.syscallThreshold = from.syscallThreshold;
            this.cfgThreshold = from.cfgThreshold;
            this.cfgLevel = from.cfgLevel;
            this.cfDirection = from.cfDirection;
            this.dfgThreshold = from.dfgThreshold;
            this.dfgLevel = from.dfgLevel;
            this.dfDirection = from.dfDirection;
            this.maxMatches = from.maxMatches;
            this.whenToMatch = from.whenToMatch;
            this.code = from.code;
            this.data = from.data;
            this.syscalls = from.syscalls;
            this.dataFlow = from.dataFlow;
            this.controlFlow = from.controlFlow;
            this.frequency = from.frequency;
            this.order = from.order;
            this.codePareto = from.codePareto;
            this.dataPareto = from.dataPareto;
            this.syscallsPareto = from.syscallsPareto;
            this.dataFlowPareto = from.dataFlowPareto;
            this.controlFlowPareto = from.controlFlowPareto;
            this.frequencyPareto = from.frequencyPareto;
            this.orderPareto = from.orderPareto;
        }

        public static void SaveListToFile(List<Settings> toSave, String fileName)
        {
            System.IO.FileStream fs = new System.IO.FileStream(fileName, System.IO.FileMode.Create);

            System.Runtime.Serialization.Formatters.Binary.BinaryFormatter formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
            try
            {
                formatter.Serialize(fs, toSave);
            }
            catch (System.Runtime.Serialization.SerializationException f)
            {
                Console.WriteLine("Failed to serialize. Reason: " + f.Message);
                throw;
            }
            finally
            {
                fs.Close();
            }
        }

        public static List<Settings> LoadListFromFile(String fileName)
        {
            List<Settings> loaded;

            System.IO.FileStream fs = new System.IO.FileStream(fileName, System.IO.FileMode.Open);

            System.Runtime.Serialization.Formatters.Binary.BinaryFormatter formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
            try
            {
                loaded = (List<Settings>)formatter.Deserialize(fs);
            }
            catch (System.Runtime.Serialization.SerializationException f)
            {
                Console.WriteLine("Failed to serialize. Reason: " + f.Message);
                throw;
            }
            finally
            {
                fs.Close();
            }
            return loaded;
        }

    }
    public class Comparison
    {
        public Comparison(string dirProgram1, string dirProgram2)
        {
            //this = new Comparison();
            RegisterProgram1(dirProgram1 + @"\instructions.xml");
            RegisterProgram2(dirProgram2 + @"\instructions.xml");

            if (System.IO.File.Exists(dirProgram1 + @"\mapping.xml"))
            {
                RegisterMapping1(dirProgram1 + @"\mapping.xml");
                RegisterMapping2(dirProgram2 + @"\mapping.xml");
            }
            else
            {
                FakeMapping(program1);
                FakeMapping(program2);
            }

            DetermineCorrectMapping();

            string tmp;
            if (System.IO.File.Exists(dirProgram1 + @"\cfg.xml"))
                tmp = @"\cfg.xml";
            else if (System.IO.File.Exists(dirProgram1 + @"\dyncfg.xml"))
                tmp = @"\dyncfg.xml";
            else throw new Exception();

            RegisterCfg1(dirProgram1 + tmp);
            RegisterCfg2(dirProgram2 + tmp);

            if (System.IO.File.Exists(dirProgram1 + @"\syscalls.xml"))
            {
                RegisterSyscalls1(dirProgram1 + @"\syscalls.xml");
                RegisterSyscalls2(dirProgram2 + @"\syscalls.xml");
            }
            if (System.IO.File.Exists(dirProgram1 + @"\trace-values.xml"))
            {
                RegisterValues1(dirProgram1 + @"\trace-values.xml");
                RegisterValues2(dirProgram2 + @"\trace-values.xml");
            }
            if (System.IO.File.Exists(dirProgram1 + @"\memslice.xml"))
            {
                RegisterDependencyGraph1(dirProgram1 + @"\memslice.xml");
                RegisterDependencyGraph2(dirProgram2 + @"\memslice.xml");
            }
            if (System.IO.File.Exists(dirProgram1 + @"\regslice.xml"))
            {
                RegisterDependencyGraph1(dirProgram1 + @"\regslice.xml");
                RegisterDependencyGraph2(dirProgram2 + @"\regslice.xml");
            }
            if (System.IO.File.Exists(dirProgram1 + @"\regdepsalways.xml"))
            {
                RegisterDependencyGraph1(dirProgram1 + @"\regdepsalways.xml");
                RegisterDependencyGraph2(dirProgram2 + @"\regdepsalways.xml");
            }
        }

        private bool dgRead = false;
        // Set by RegisterValues1 (trace-values.xml)
        private bool valuesRead = false;
        // Set by RegisterSyscalls1 (syscalls.xml)
        private bool syscallsRead = false;

        //private System.Collections.Generic.List<Syscall> program1Syscalls;
        private System.Collections.Generic.Dictionary<Instruction, List<Syscall>> program1Syscalls;

        public ControlFlowGraph program1Cfg;
        public System.Collections.Generic.SortedList<Int32, Instruction> program1;

        //private System.Collections.Generic.List<Syscall> program2Syscalls;
        private System.Collections.Generic.Dictionary<Instruction, List<Syscall>> program2Syscalls;

        public ControlFlowGraph program2Cfg;
        public System.Collections.Generic.SortedList<Int32, Instruction> program2;

        // Data dependency graph: connects instructions to the instructions that last defined the values used
        private void ReadDependencyGraph(string filename, SortedList<int, Instruction> program)
        {
            XmlTextReader reader = new XmlTextReader(filename);

            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (reader.Name == "dependency")
                        {
                            Int32 from = System.Int32.Parse(reader.GetAttribute("from"), System.Globalization.NumberStyles.HexNumber);
                            Int32 to = System.Int32.Parse(reader.GetAttribute("to"), System.Globalization.NumberStyles.HexNumber);
                            if (program.ContainsKey(from) && program.ContainsKey(to))
                            {
							    // Avoid duplicates
                                if (!program[from].backwardDependencies.Contains(program[to]))
                                    program[from].backwardDependencies.Add(program[to]);
                                if (!program[to].forwardDependencies.Contains(program[from]))
                                    program[to].forwardDependencies.Add(program[from]);
                            }
                        }
                        break;
                }
                reader.MoveToElement();
            }
            reader.Close();
        }

        /*private System.Collections.Generic.List<Syscall> ReadSyscalls(string p)
        {
            System.Collections.Generic.List<Syscall> syscallList = new List<Syscall>();
            Syscall lastSyscall = null;
            XmlTextReader reader = new XmlTextReader(p);
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (reader.Name == "syscall")
                        {
                            lastSyscall = new Syscall();
                            lastSyscall.ip = System.Int32.Parse(reader.GetAttribute("ip"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[0] = System.Int32.Parse(reader.GetAttribute("eax"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[1] = System.Int32.Parse(reader.GetAttribute("ebx"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[2] = System.Int32.Parse(reader.GetAttribute("ecx"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[3] = System.Int32.Parse(reader.GetAttribute("edx"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[4] = System.Int32.Parse(reader.GetAttribute("ebp"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[5] = System.Int32.Parse(reader.GetAttribute("esp"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[6] = System.Int32.Parse(reader.GetAttribute("esi"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[7] = System.Int32.Parse(reader.GetAttribute("edi"), System.Globalization.NumberStyles.HexNumber);
                            syscallList.Add(lastSyscall);
                        }
                        else if (reader.Name == "caller")
                        {
                            lastSyscall.callStack.Push(System.Int32.Parse(reader.GetAttribute("address"), System.Globalization.NumberStyles.HexNumber));
                        }
                        break;
                }
                reader.MoveToElement();
            }
            reader.Close();
            return syscallList;
        }*/


        private void ReadValues(string filename, SortedList<int, Instruction> program)
        {
			// NOTE: could be merged with ReadDependencyGraph
            XmlTextReader reader = new XmlTextReader(filename);
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (reader.Name == "before" || reader.Name == "after")
                        {
                            RegisterSetValues values = new RegisterSetValues();
                            Int32 address = System.Int32.Parse(reader.GetAttribute("ip"), System.Globalization.NumberStyles.HexNumber);
                            values.registers[0] = System.UInt32.Parse(reader.GetAttribute("eax"), System.Globalization.NumberStyles.HexNumber);
                            values.registers[1] = System.UInt32.Parse(reader.GetAttribute("ebx"), System.Globalization.NumberStyles.HexNumber);
                            values.registers[2] = System.UInt32.Parse(reader.GetAttribute("ecx"), System.Globalization.NumberStyles.HexNumber);
                            values.registers[3] = System.UInt32.Parse(reader.GetAttribute("edx"), System.Globalization.NumberStyles.HexNumber);
                            values.registers[4] = System.UInt32.Parse(reader.GetAttribute("esi"), System.Globalization.NumberStyles.HexNumber);
                            values.registers[5] = System.UInt32.Parse(reader.GetAttribute("edi"), System.Globalization.NumberStyles.HexNumber);
						    values.registers[6] = System.UInt32.Parse(reader.GetAttribute("ebp"), System.Globalization.NumberStyles.HexNumber);
                            values.registers[7] = System.UInt32.Parse(reader.GetAttribute("esp"), System.Globalization.NumberStyles.HexNumber);
						    // If this address exists in our program, add these values.
                            if (program.ContainsKey(address))
                                program[address].addValue(values);
                        }
                        if (reader.Name == "data")
                        {
                            //<data ip="b7f7990e" type="1" length="4" address="bf8978bc" value="b7f7990e"
                            Int32 address = System.Int32.Parse(reader.GetAttribute("ip"), System.Globalization.NumberStyles.HexNumber);
                            UInt32 value = System.UInt32.Parse(reader.GetAttribute("value"), System.Globalization.NumberStyles.HexNumber);
						    // Avoid duplication
                            if (program.ContainsKey(address) && Comparison.IntIsDiscriminativeAndInvariant(value) && !program[address].values.Contains(value))
                                program[address].values.Add(value);

                        }
                        break;
                }
                reader.MoveToElement();
            }
            reader.Close();
        }

        private Dictionary<Instruction, List<Syscall>> ReadSyscalls(string filename, SortedList<Int32, Instruction> program)
        {
            Dictionary<Instruction, List<Syscall>> syscallList = new Dictionary<Instruction, List<Syscall>>();

            Syscall lastSyscall = null;

            XmlTextReader reader = new XmlTextReader(filename);

            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (reader.Name == "syscall")
                        {
                            lastSyscall = new Syscall();
                            lastSyscall.ip = System.Int32.Parse(reader.GetAttribute("ip"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[0] = System.UInt32.Parse(reader.GetAttribute("eax"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[1] = System.UInt32.Parse(reader.GetAttribute("ebx"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[2] = System.UInt32.Parse(reader.GetAttribute("ecx"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[3] = System.UInt32.Parse(reader.GetAttribute("edx"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[4] = System.UInt32.Parse(reader.GetAttribute("esi"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[5] = System.UInt32.Parse(reader.GetAttribute("edi"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[6] = System.UInt32.Parse(reader.GetAttribute("ebp"), System.Globalization.NumberStyles.HexNumber);
                            lastSyscall.registers[7] = System.UInt32.Parse(reader.GetAttribute("esp"), System.Globalization.NumberStyles.HexNumber);
                            {
                                // Search until we find an instruction with opcode "int"
                                Instruction ins = program[lastSyscall.ip];
                                if (ins.opcode != "int")
                                {
                                    foreach (Instruction i in ins.basicBlock.instructions)
                                    {
                                        if (i.opcode == "int")
                                        {
                                            ins = i;
                                            break;
                                        }
                                    }
                                }
                                if (ins.opcode != "int")
                                {
                                    throw new Exception("Could not find int instruction for syscall.");
                                }
                                if (!syscallList.ContainsKey(ins))
                                    syscallList[ins] = new List<Syscall>();
                                syscallList[ins].Add(lastSyscall);
                            }
                        }
                        else if (reader.Name == "caller")
                        {
                            lastSyscall.callStack.Push(System.Int32.Parse(reader.GetAttribute("address"), System.Globalization.NumberStyles.HexNumber));
                        }
                        break;
                }
                reader.MoveToElement();
            }
            reader.Close();

            return syscallList;
        }

        private ControlFlowGraph ReadCfgAndAddInstructions(string filename, SortedList<int, Instruction> program)
        {
            ControlFlowGraph cfg = new ControlFlowGraph();

            XmlTextReader reader = new XmlTextReader(filename);

            // First loop the file contents and add the basic blocks to the cfg. No connections yet.
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (reader.Name == "bbl")
                        {
                            Int32 from = System.Int32.Parse(reader.GetAttribute("from_bbl").Substring(2), System.Globalization.NumberStyles.HexNumber);
                            Int32 to = System.Int32.Parse(reader.GetAttribute("to_bbl").Substring(2), System.Globalization.NumberStyles.HexNumber);
                            Int32 nr = System.Int32.Parse(reader.GetAttribute("nr"));
                            Int32 count = System.Int32.Parse(reader.GetAttribute("count"));

                            /*deals with blocks that have been split by diota after they were first dealt with as a single basic block*/
                            if (!cfg.blocks.ContainsKey(from))
                            {
                                cfg.blocks.Add(from, new BasicBlock(from, nr, count));
                            }
                        }
                        break;
                    /*
                    BasicBlock b = cfg.blocks[from];
                    if (to < b.to)
                    {
                        //fix strange diota behavior
                        to++;
                        while (!program.ContainsKey(to))
                            to++;
                        to--;
                        b.from = to + 1;
                        cfg.blocks.Remove(from);
                        cfg.blocks.Add(b.from, b);
                        BasicBlock c = new BasicBlock(from, to, nr, count);
                        cfg.blocks.Add(from, c);
                        if (b.predecessors.Count != 0)
                        {
                            c.predecessors = b.predecessors;
                            b.predecessors = new List<BasicBlock>();
                        }
                        c.successors.Add(b);
                        b.predecessors.Add(c);
                    }
                    //occurs because the bbls are not written out in the order in which they were first processed
                    else if (to > b.to)
                    {
                        if (b.successors.Count == 0)
                        {
                            from = b.to + 1;
                            //fix strange diota behavior
                            while (!program.ContainsKey(from))
                                from++;
                            b.to = from - 1;
                            BasicBlock c = new BasicBlock(from, to, nr, count);
                            cfg.blocks.Add(from, c);
                            b.successors.Add(c);
                            c.predecessors.Add(b);
                        }
                    }
                    //otherwise it has already been added through splitting
                }
                else
                {
                    cfg.blocks.Add(from, new BasicBlock(from, to, nr, count));
                }*/
                }
                reader.MoveToElement();
            }
            reader.Close();
            /*
            Dictionary<Int32, BasicBlock> lastAddressToBasicBlock = new Dictionary<int, BasicBlock>();
            foreach (BasicBlock b in cfg.blocks.Values)
            {
                //deals with blocks that have been split by diota after they were first dealt with as a single basic block
                if (lastAddressToBasicBlock.ContainsKey(b.to))
                {
                    BasicBlock c = lastAddressToBasicBlock[b.to];
                    if (c.from < b.from)
                    {
                        lastAddressToBasicBlock.Remove(b.to);
                        c.to = b.from - 1;
                        lastAddressToBasicBlock.Add(c.to, c);
                        lastAddressToBasicBlock.Add(b.to, b);
                        c.successors.Add(b);
                        b.predecessors.Add(c);
                    }
                    else
                    {
                        b.to = c.from - 1;
                        lastAddressToBasicBlock.Add(b.to, b);
                        b.successors.Add(c);
                        c.predecessors.Add(b);
                    }
                }
                else
                {
                    lastAddressToBasicBlock.Add(b.to, b);
                }
            }
            */
            cfg.AddInstructions(program);

            /*
            List<BasicBlock> toRemove = new List<BasicBlock>();
            foreach (BasicBlock b in cfg.blocks.Values)
            {
                if (b.instructions.Count == 0)
                {
                    foreach (BasicBlock c in b.successors)
                    {
                        foreach (BasicBlock d in b.predecessors)
                        {
                            d.successors.Add(c);
                            c.predecessors.Add(d);
                        }
                    }
                    toRemove.Add(b);
                    System.Console.WriteLine(" Warning: empty basic block" + b.from.ToString("x"));
                }
            }
            foreach (BasicBlock b in toRemove)
            {
                foreach (BasicBlock c in b.predecessors)
                {
                    c.successors.Remove(b);
                }
                foreach (BasicBlock c in b.successors)
                {
                    c.predecessors.Remove(b);
                }
                cfg.blocks.Remove(b.from);
            }
            */

            // Now create the connections.
            reader = new XmlTextReader(filename);
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (reader.Name == "edge")
                        {
                            Int32 from = System.Int32.Parse(reader.GetAttribute("from_edge").Substring(2), System.Globalization.NumberStyles.HexNumber);
                            Int32 to = System.Int32.Parse(reader.GetAttribute("to_edge").Substring(2), System.Globalization.NumberStyles.HexNumber);
                            Int32 count = System.Int32.Parse(reader.GetAttribute("count"));
                            //System.Console.WriteLine("edge from" + from + " to " + to + "count" + count);
                            //fix strange diota behavior
                            while (!cfg.blocks.ContainsKey(from))
                                from--;
                            if (!cfg.blocks[from].successors.Contains(cfg.blocks[to]))
                            {
                                cfg.blocks[from].successors.Add(cfg.blocks[to]);
                                cfg.blocks[to].predecessors.Add(cfg.blocks[from]);
                            }
                        }
                        break;
                }
                reader.MoveToElement();
            }
            reader.Close();

            for (int i = 0; i < cfg.blocks.Count; i++)
                cfg.blocks.Values[i].indexInCfg = i;

            return cfg;
        }
        
        private SortedList<Int32, Instruction> ReadInstructions(string filename)
        {
            SortedList<Int32, Instruction> instructionList = new SortedList<Int32, Instruction>();
            Instruction lastIns = null;

            XmlTextReader reader = new XmlTextReader(filename);
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (reader.Name == "ins")
                        {
                            string assembly = reader.GetAttribute("assembly");
                            Int32 address = System.Int32.Parse(reader.GetAttribute("address").Substring(2), System.Globalization.NumberStyles.HexNumber);
                            string opcode = reader.GetAttribute("opcode");
                            // Avoid duplicates
                            if (!instructionList.ContainsKey(address))
                                instructionList.Add(address, new Instruction(assembly, address, opcode));
                            //else
                                // System.Console.WriteLine("problem with" + address + p);
                            lastIns = instructionList[address];

                        }
                        else if (reader.Name == "source1" || reader.Name == "source2" || reader.Name == "dest")
                        {
                            // Information about the previous instruction (lastIns)
                            Operand o = new Operand();
                            string type = reader.GetAttribute("type");
                            switch (type)
                            {
                                case "none":
                                    break;
                                case "reg":
                                    o.type = OperandType.reg;
                                    o.baseReg = stringToReg(reader.GetAttribute("reg"));
                                    break;
                                case "memory":
                                    o.type = OperandType.mem;
                                    o.immediate = (UInt32)System.Int32.Parse(reader.GetAttribute("immediate"));
                                    o.baseReg = stringToReg(reader.GetAttribute("base"));
                                    o.indexReg = stringToReg(reader.GetAttribute("index"));
                                    // TODO: scale?
                                    break;
                                case "immediate":
                                    o.type = OperandType.imm;
                                    o.immediate = (UInt32)System.Int32.Parse(reader.GetAttribute("immediate"));
                                    break;
                                default:
                                    throw new Exception("Unknown operand");
                            }

                            if (reader.Name == "source1")
                                lastIns.Source1 = o;
                            else if (reader.Name == "source2")
                                lastIns.Source2 = o;
                            else if (reader.Name == "dest")
                                lastIns.dest = o;
                        }
                        break;
                }
                reader.MoveToElement();
            }
            reader.Close();
            return instructionList;
        }

        private UInt32 stringToReg(string reg)
        {
            switch (reg)
            {
                case "%eax":
                    return 0x1;
                case "%ax":
                    return 0x101;
                case "%al":
                    return 0x201;
                case "%ah":
                    return 0x301;

                case "%ebx":
                    return 0x2;
                case "%bx":
                    return 0x102;
                case "%bl":
                    return 0x202;
                case "%bh":
                    return 0x302;

                case "%ecx":
                    return 0x4;
                case "%cx":
                    return 0x104;
                case "%cl":
                    return 0x204;
                case "%ch":
                    return 0x304;

                case "%edx":
                    return 0x8;
                case "%dx":
                    return 0x108;
                case "%dl":
                    return 0x208;
                case "%dh":
                    return 0x38;

                case "%ebp":
                    return 0x10;
                case "%bp":
                    return 0x110;

                case "%esp":
                    return 0x20;

                case "%edi":
                    return 0x40;
                case "%di":
                    return 0x140;

                case "%esi":
                    return 0x80;
                case "%si":
                    return 0x180;

                case "%st":
                    return 0x100;
                case "%st1":
                    return 0x200;
                case "%st3":
                    return 0x300;
                case "%st2":
                    return 0x400;
                case "%st4":
                    return 0x500;
                case "%st5":
                    return 0x600;
                case "%st6":
                    return 0x700;
                case "%st7":
                    return 0x800;

                case "":
                    return 0xFF00;

                default:
                    throw new Exception();
            }
        }
        private void ReadMapping(string filename, SortedList<Int32, Instruction> list)
        {
            XmlTextReader reader = new XmlTextReader(filename);
            int address = 0;
            List<int> addressList = null;
            string s = null;
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (reader.Name == "ins")
                        {
                            address = System.Int32.Parse(reader.GetAttribute("address").Substring(2), System.Globalization.NumberStyles.HexNumber);
                            addressList = new List<int>();
                        }
                        break;

                    case XmlNodeType.Text:
                        s = reader.ReadString();
                        char[] c ={ ' ' };
                        string[] strings = s.Split(c);
                        foreach (String iter in strings)
                        {
                            if (iter == "")
                                continue;
                            addressList.Add(System.Int32.Parse(iter.Substring(2), System.Globalization.NumberStyles.HexNumber));
                        }
                        if (list.ContainsKey(address))//mapping is from static version, some instructions do not appear in an execution
			            {
                            list[address].oldAddresses = addressList;
			            }
                        break;
                }
                // TODO: Infinite loop??
                //reader.MoveToElement();
            }
            reader.Close();
        }

        private void FakeMapping(SortedList<Int32, Instruction> list)
        {
            int i = 1;
            foreach (Instruction ins in list.Values)
            {
                List<int> addressList = new List<int>();
                addressList.Add(i++);
                list[ins.address].oldAddresses = addressList;
            }
        }
        
        // Two Instruction Lists...
        class TwoLists
        {
            public List<Instruction> list1;
            public List<Instruction> list2;
            public TwoLists()
            {
                list1 = new List<Instruction>();
                list2 = new List<Instruction>();
            }
        }

        /*
         * Methods relating the actual matching.
         */
        public void DetermineCorrectMapping()
        {
            Dictionary<Int32, TwoLists> dictionary = new Dictionary<int, TwoLists>();
            // Fill list1
            foreach (KeyValuePair<Int32, Instruction> pair in program1)
            {
                foreach (Int32 oldAddress in pair.Value.oldAddresses)
                {
                    if (oldAddress == 0)
                        continue;
                    if (!dictionary.ContainsKey(oldAddress))
                        dictionary[oldAddress] = new TwoLists();
                    dictionary[oldAddress].list1.Add(pair.Value);
                }
            }
            // Fill list2
            foreach (KeyValuePair<Int32, Instruction> pair in program2)
            {
                foreach (Int32 oldAddress in pair.Value.oldAddresses)
                {
                    if (oldAddress == 0)
                        continue;
                    if (!dictionary.ContainsKey(oldAddress))
                        dictionary[oldAddress] = new TwoLists();
                    dictionary[oldAddress].list2.Add(pair.Value);
                }
            }
            // Map list1 and list2
            foreach (KeyValuePair<Int32, TwoLists> pair in dictionary)
            {
                foreach (Instruction ins1 in pair.Value.list1)
                    foreach (Instruction ins2 in pair.Value.list2)
                    {
                        // Bi-directional mapping
                        AddRequiredMapping(ins1, ins2);
                    }
            }
        }

        private void AddGuessedMapping(Instruction instruction, Instruction instruction_2)
        {
            instruction.addGuessedMapping(instruction_2);
            instruction_2.addGuessedMapping(instruction);
        }

        private void RemoveGuessedMapping(Instruction instruction, Instruction instruction_2)
        {
            instruction.removeGuessedMapping(instruction_2);
            instruction_2.removeGuessedMapping(instruction);
        }

        private void AddRequiredMapping(Instruction instruction, Instruction instruction_2)
        {
            instruction.addRequiredMapping(instruction_2);
            instruction_2.addRequiredMapping(instruction);
        }

        public int NrOk
        {
            get
            {
                int nrOk = 0;
                foreach (KeyValuePair<Int32, Instruction> pair in program1)
                {
                    nrOk += pair.Value.nrOk;
                }
                return nrOk;
            }
        }

        public double oKRate
        {
            get
            {
                return (NrOk * 1.0 / (NrOk + NrFn));
            }
        }
        public double fnRate
        {
            get
            {
                return (NrFn * 1.0 / (NrOk + NrFn));
            }
        }
        public double fpRate
        {
            get
            {
                return (NrFp * 1.0 / (NrOk + NrFp));
            }
        }
        

        public int NrFp
        {
            get
            {
                int nrFp = 0;
                foreach (KeyValuePair<Int32, Instruction> pair in program1)
                {
                    nrFp += pair.Value.nrFp;
                }
                return nrFp;
            }
        }
        public int NrFn
        {
            get
            {
                int nrFn = 0;
                foreach (KeyValuePair<Int32, Instruction> pair in program1)
                {
                    nrFn += pair.Value.nrFn;
                }
                return nrFn;
            }
        }

        internal List<Instruction> getAllFalseNegatives()
        {
            List<Instruction> list = new List<Instruction>();
            foreach (KeyValuePair<Int32, Instruction> pair in program1)
            {
                if (pair.Value.getFalseNegatives().Count != 0)
                    list.Add(pair.Value);
            }
            return list;
        }

        internal List<Instruction> getAllFalsePositives()
        {
            List<Instruction> list = new List<Instruction>();
            foreach (KeyValuePair<Int32, Instruction> pair in program1)
            {
                if (pair.Value.getFalsePositives().Count != 0)
                    list.Add(pair.Value);
            }
            return list;
        }

        public List<Instruction> getCorrect()
        {
            List<Instruction> list = new List<Instruction>();

            foreach (KeyValuePair<Int32, Instruction> pair in program1)
            {
                if (pair.Value.correct)
                    list.Add(pair.Value);
            }
            return list;
        }

        public List<Instruction> getMapped()
        {
            List<Instruction> list = new List<Instruction>();

            foreach (KeyValuePair<Int32, Instruction> pair in program1)
            {
                if (pair.Value.mapped)
                    list.Add(pair.Value);
            }
            return list;
        }

        // TODO: Should be connected to the Swap button in the Inspection window.
        public void Swap()
        {
            System.Collections.Generic.SortedList<Int32, Instruction> tmp = program1;
            program1 = program2;
            program2 = tmp;
        }

        internal void RegisterProgram1(string fileName)
        {
            program1 = ReadInstructions(fileName);

            if (program2 != null)
            {
                foreach (KeyValuePair<Int32, Instruction> pair in program2)
                {
                    pair.Value.clearMappings();
                }
            }
        }

        internal void RegisterProgram2(string fileName)
        {
            program2 = ReadInstructions(fileName);

            if (program1 != null)
            {
                foreach (KeyValuePair<Int32, Instruction> pair in program1)
                {
                    pair.Value.clearMappings();
                }
            }
        }

        internal void RegisterMapping1(string filename)
        {
            ReadMapping(filename, program1);
        }
        internal void RegisterMapping2(string filename)
        {
            ReadMapping(filename, program2);
        }
        internal void RegisterCfg2(string filename)
        {
            program2Cfg = ReadCfgAndAddInstructions(filename, program2);
        }
        internal void RegisterCfg1(string filename)
        {
            program1Cfg = ReadCfgAndAddInstructions(filename, program1);
        }

        internal void RegisterSyscalls1(string filename)
        {
            syscallsRead = true;
            program1Syscalls = ReadSyscalls(filename, program1);
        }
        internal void RegisterSyscalls2(string filename)
        {
            program2Syscalls = ReadSyscalls(filename, program2);
            //program2Syscalls = ReadSyscalls(filename);
        }

        internal void RegisterDependencyGraph1(string filename)
        {
            dgRead = true;
            ReadDependencyGraph(filename, program1);
        }
        internal void RegisterDependencyGraph2(string filename)
        {
            ReadDependencyGraph(filename, program2);
        }
        internal void RegisterValues1(string filename)
        {
            valuesRead = true;
            ReadValues(filename, program1);
        }
        internal void RegisterValues2(string filename)
        {
            ReadValues(filename, program2);
        }

        public void TransFilterMapping(Settings settings)
        {
            foreach (BasicBlock b in program1Cfg.blocks.Values)
            {
                foreach (BasicBlock c in b.mapping)
                {
                    foreach (Instruction i in b.instructions)
                    {
                        Double minCodeScore = 1;
                        Double minDataScore = 1;
                        Double minSyscallScore = 1;
                        Double minCodeFlowScore = 1;
                        Double minDataFlowScore = 1;
                        Double minFrequencyScore = 1;
                        Double minOrderScore = 1;

                        Instruction worst = null;
                        List<Instruction> toUnmatch = null;
                        foreach (Instruction j in i.mapping.Values)
                        {
                            Double codeScore = 1;
                            Double dataScore = 1;
                            Double syscallScore = 1;
                            Double codeFlowScore = 1;
                            Double dataFlowScore = 1;
                            Double frequencyScore = 1;
                            Double orderScore = 1;


                            if (settings.order)
                            {
                                orderScore = CompareInsByOrder(i, j);
                                if (orderScore >= settings.orderThreshold)
                                    continue;
                            }
                            if (settings.frequency)
                            {
                                frequencyScore = CompareInsByFrequency(i, j);
                                if (frequencyScore >= settings.execThreshold)
                                    continue;
                            }
                            if (settings.code)
                            {
                                codeScore = CompareInsBySyntax(i, j);
                                if (codeScore >= settings.insThreshold)
                                    continue;
                            }
                            if (settings.data)
                            {
                                dataScore = CompareInsByDataOrMinusOne(i, j);
                                if (dataScore == -1 || dataScore >= settings.dataThreshold)
                                    continue;
                            }
                            if (settings.controlFlow)
                            {
                                codeFlowScore = CompareInsByControlFlow(i, j, settings.cfDirection, settings.cfgLevel);
                                if (codeFlowScore >= settings.cfgThreshold)
                                    continue;
                            }
                            if (settings.dataFlow)
                            {
                                dataFlowScore = CompareInsByDataFlowOrMinusOne(i, j, settings.dfDirection, settings.dfgLevel);
                                if (dataFlowScore == -1 || dataFlowScore >= settings.dfgThreshold)
                                    continue;
                            }
                            if (settings.syscalls)
                            {
                                syscallScore = CompareInsBySyscallsOrMinusOne(i, j);
                                if (syscallScore == -1 || syscallScore >= settings.syscallThreshold)
                                    continue;
                            }

                            // If the match was good for one of the selected settings
                            // the code wouldn't get here (continue starts the next loop).
                            // So... We delete the bad match!
                            if (settings.whenToMatch == WhenToMatchOrUnmatch.ALL)
                            {
                                if (toUnmatch == null)
                                    toUnmatch = new List<Instruction>();
                                toUnmatch.Add(j);
                            }
                            else if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST)
                            {
                                if ((!settings.codePareto || codeScore <= minCodeScore) &&
                                    (!settings.dataPareto || dataScore <= minDataScore) &&
                                    (!settings.syscallsPareto || syscallScore <= minSyscallScore) &&
                                    (!settings.controlFlowPareto || codeFlowScore <= minCodeFlowScore) &&
                                    (!settings.dataFlowPareto || dataFlowScore <= minDataFlowScore) &&
                                    (!settings.frequencyPareto || frequencyScore <= minFrequencyScore) &&
                                    (!settings.orderPareto || orderScore <= minOrderScore))
                                {
                                    minCodeScore = codeScore;
                                    minDataScore = dataScore;
                                    minSyscallScore = syscallScore;
                                    minCodeFlowScore = codeFlowScore;
                                    minDataFlowScore = dataFlowScore;
                                    minFrequencyScore = frequencyScore;
                                    minOrderScore = orderScore;
                                    worst = j;
                                }
                            }
                            else
                            {
                                throw new Exception("Unsupported");
                            }
                        }
                        if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST && worst != null)
                        {
                            RemoveGuessedMapping(i, worst);
                        }
                        else if (settings.whenToMatch == WhenToMatchOrUnmatch.ALL)
                        {
                            if (toUnmatch != null)
                                foreach (Instruction k in toUnmatch)
                                {
                                    RemoveGuessedMapping(i, k);
                                }
                        }
                    }
                }
            }
        }

        public void InsFilterMapping(Settings settings)
        {
            foreach (Instruction i in program1.Values)
            {
                if (!i.mapped)
                    continue;
                
                // Initialize minimum values
                Double minCodeScore = 1;
                Double minDataScore = 1;
                Double minSyscallScore = 1;
                Double minCodeFlowScore = 1;
                Double minDataFlowScore = 1;
                Double minFrequencyScore = 1;
                Double minOrderScore = 1;

                Instruction worst = null;
                List<Instruction> toUnmatch = null;
                foreach (Instruction j in i.mapping.Values)
                {
                    Double codeScore = 1;
                    Double dataScore = 1;
                    Double syscallScore = 1;
                    Double codeFlowScore = 1;
                    Double dataFlowScore = 1;
                    Double frequencyScore = 1;
                    Double orderScore = 1;


                    if (settings.order)
                    {
                        orderScore = CompareInsByOrder(i, j);
                        if (orderScore >= settings.orderThreshold)
                            continue;
                    }
                    if (settings.frequency)
                    {
                        frequencyScore = CompareInsByFrequency(i, j);
                        if (frequencyScore >= settings.execThreshold)
                            continue;
                    }
                    if (settings.code)
                    {
                        codeScore = CompareInsBySyntax(i, j);
                        if (codeScore >= settings.insThreshold)
                            continue;
                    }
                    if (settings.data)
                    {
                        dataScore = CompareInsByDataOrMinusOne(i, j);
                        if (dataScore == -1 || dataScore >= settings.dataThreshold)
                            continue;
                    }
                    if (settings.controlFlow)
                    {
                        codeFlowScore = CompareInsByControlFlow(i, j, settings.cfDirection, settings.cfgLevel);
                        if (codeFlowScore >= settings.cfgThreshold)
                            continue;
                    }
                    if (settings.dataFlow)
                    {
                        dataFlowScore = CompareInsByDataFlowOrMinusOne(i, j, settings.dfDirection, settings.dfgLevel);
                        if (dataFlowScore == -1 || dataFlowScore >= settings.dfgThreshold)
                            continue;
                    }
                    if (settings.syscalls)
                    {
                        syscallScore = CompareInsBySyscallsOrMinusOne(i, j);
                        if (syscallScore == -1 || syscallScore >= settings.syscallThreshold)
                            continue;
                    }


                    if (settings.whenToMatch == WhenToMatchOrUnmatch.ALL)
                    {
                        if (toUnmatch == null)
                            toUnmatch = new List<Instruction>();
                        toUnmatch.Add(j);
                    }
                    else if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST)
                    {
                        if ((!settings.codePareto || codeScore <= minCodeScore) &&
                                    (!settings.dataPareto || dataScore <= minDataScore) &&
                                    (!settings.syscallsPareto || syscallScore <= minSyscallScore) &&
                                    (!settings.controlFlowPareto || codeFlowScore <= minCodeFlowScore) &&
                                    (!settings.dataFlowPareto || dataFlowScore <= minDataFlowScore) &&
                                    (!settings.frequencyPareto || frequencyScore <= minFrequencyScore) &&
                                    (!settings.orderPareto || orderScore <= minOrderScore))
                        {
                            minCodeScore = codeScore;
                            minDataScore = dataScore;
                            minSyscallScore = syscallScore;
                            minCodeFlowScore = codeFlowScore;
                            minDataFlowScore = dataFlowScore;
                            minFrequencyScore = frequencyScore;
                            minOrderScore = orderScore;
                            worst = j;
                        }
                    }
                    else
                    {
                        throw new Exception("Unsupported");
                    }
                } // End Foreach j
                
                // Remove the worst mapping worst for instruction i
                if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST && worst != null)
                {
                    RemoveGuessedMapping(i, worst);
                }
                // Remove all mappings if WhenToMatchOrUnmatch.ALL
                else if (settings.whenToMatch == WhenToMatchOrUnmatch.ALL)
                {
                    if (toUnmatch != null)
                        foreach (Instruction j in toUnmatch)
                        {
                            RemoveGuessedMapping(i, j);
                        }
                }
            } // End foreach i
        }

        public void BblFilterMapping(Settings settings)
        {
            foreach (BasicBlock b in program1Cfg.blocks.Values)
            {
                if (b.mapping.Count == 0)
                    continue;

                Double minCodeScore = 1;
                Double minDataScore = 1;
                Double minSyscallScore = 1;
                Double minCodeFlowScore = 1;
                Double minDataFlowScore = 1;
                Double minFrequencyScore = 1;
                Double minOrderScore = 1;

                BasicBlock worst = null;
                List<BasicBlock> toUnmatch = null;
                foreach (BasicBlock c in b.mapping)
                {
                    Double codeScore = 1;
                    Double dataScore = 1;
                    Double syscallScore = 1;
                    Double codeFlowScore = 1;
                    Double dataFlowScore = 1;
                    Double frequencyScore = 1;
                    Double orderScore = 1;

                    if (settings.order)
                    {
                        orderScore = CompareBblByOrder(b, c);
                        if (orderScore >= settings.orderThreshold)
                            continue;
                    }
                    if (settings.frequency)
                    {
                        frequencyScore = CompareBblByFrequency(b, c);
                        if (frequencyScore >= settings.execThreshold)
                            continue;
                    }
                    if (settings.code)
                    {
                        codeScore = CompareBblBySyntax(b, c);
                        if (codeScore >= settings.insThreshold)
                            continue;
                    }
                    if (settings.data)
                    {
                        dataScore = CompareBblByDataOrMinusOne(b, c);
                        if (dataScore == -1 || dataScore >= settings.dataThreshold)
                            continue;
                    }
                    if (settings.controlFlow)
                    {
                        codeFlowScore = CompareBblByControlFlow(b, c, settings.cfDirection, settings.cfgLevel);
                        if (codeFlowScore >= settings.cfgThreshold)
                            continue;
                    }
                    if (settings.dataFlow)
                    {
                        dataFlowScore = CompareBblByDataFlowOrMinusOne(b, c, settings.dfDirection, settings.dfgLevel);
                        if (dataFlowScore == -1 || dataFlowScore >= settings.dfgThreshold)
                            continue;
                    }
                    if (settings.syscalls)
                    {
                        syscallScore = CompareBblBySyscallsOrMinusOne(b, c);
                        if (syscallScore == -1 || syscallScore >= settings.syscallThreshold)
                            continue;
                    }


                    if (settings.whenToMatch == WhenToMatchOrUnmatch.ALL)
                    {
                        if (toUnmatch == null)
                            toUnmatch = new List<BasicBlock>();
                        toUnmatch.Add(c);
                    }

                    else if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST)
                    {
                        if ((!settings.codePareto || codeScore <= minCodeScore) &&
                                    (!settings.dataPareto || dataScore <= minDataScore) &&
                                    (!settings.syscallsPareto || syscallScore <= minSyscallScore) &&
                                    (!settings.controlFlowPareto || codeFlowScore <= minCodeFlowScore) &&
                                    (!settings.dataFlowPareto || dataFlowScore <= minDataFlowScore) &&
                                    (!settings.frequencyPareto || frequencyScore <= minFrequencyScore) &&
                                    (!settings.orderPareto || orderScore <= minOrderScore))
                        {
                            minCodeScore = codeScore;
                            minDataScore = dataScore;
                            minSyscallScore = syscallScore;
                            minCodeFlowScore = codeFlowScore;
                            minDataFlowScore = dataFlowScore;
                            minFrequencyScore = frequencyScore;
                            minOrderScore = orderScore;
                            worst = c;
                        }
                    }
                    else
                    {
                        throw new Exception("Unsupported");
                    }
                }
                if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST && worst != null)
                    UnmatchInstructionsOfBasicBlocks(b, worst);
                else if (settings.whenToMatch == WhenToMatchOrUnmatch.ALL)
                {
                    if (toUnmatch != null)
                        foreach (BasicBlock c in toUnmatch)
                        {
                            UnmatchInstructionsOfBasicBlocks(b, c);
                        }
                }
            }
        }

        public void TransExtendMapping(Settings settings)
        {
            foreach (BasicBlock b in program1Cfg.blocks.Values)
            {
                foreach (BasicBlock c in b.mapping)
                {
                    foreach (Instruction i in b.instructions)
                    {
                        if (i.mapping.Count >= settings.maxMatches)
                            continue;
                        
                        // Initialize maximum scores
                        Double maxCodeScore = 0;
                        Double maxDataScore = 0;
                        Double maxSyscallScore = 0;
                        Double maxCodeFlowScore = 0;
                        Double maxDataFlowScore = 0;
                        Double maxFrequencyScore = 0;
                        Double maxOrderScore = 0;

                        Instruction best = null;

                        foreach (Instruction j in c.instructions)
                        {
                            if (j.mapping.Count >= settings.maxMatches)
                                continue;
                            if (i.mapping.Count >= settings.maxMatches)
                                break;

                            Double codeScore = 0;
                            Double dataScore = 0;
                            Double syscallScore = 0;
                            Double codeFlowScore = 0;
                            Double dataFlowScore = 0;
                            Double frequencyScore = 0;
                            Double orderScore = 0;


                            if (settings.order)
                            {
                                orderScore = CompareInsByOrder(i, j);
                                if (orderScore < settings.orderThreshold)
                                    continue;
                            }
                            if (settings.frequency)
                            {
                                frequencyScore = CompareInsByFrequency(i, j);
                                if (frequencyScore < settings.execThreshold)
                                    continue;
                            }
                            if (settings.code)
                            {
                                codeScore = CompareInsBySyntax(i, j);
                                if (codeScore < settings.insThreshold)
                                    continue;
                            }
                            if (settings.data)
                            {
                                dataScore = CompareInsByDataOrMinusOne(i, j);
                                if (dataScore == -1 || dataScore < settings.dataThreshold)
                                    continue;
                            }
                            if (settings.controlFlow)
                            {
                                codeFlowScore = CompareInsByControlFlow(i, j, settings.cfDirection, settings.cfgLevel);
                                if (codeFlowScore < settings.cfgThreshold)
                                    continue;
                            }
                            if (settings.dataFlow)
                            {
                                dataFlowScore = CompareInsByDataFlowOrMinusOne(i, j, settings.dfDirection, settings.dfgLevel);
                                if (dataFlowScore == -1 || dataFlowScore < settings.dfgThreshold)
                                    continue;
                            }
                            if (settings.syscalls)
                            {
                                syscallScore = CompareInsBySyscallsOrMinusOne(i, j);
                                if (syscallScore == -1 || syscallScore < settings.syscallThreshold)
                                    continue;
                            }

                            if (settings.whenToMatch == WhenToMatchOrUnmatch.ALL)
                            {
                                AddGuessedMapping(i, j);
                            }
                            else if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST)
                            {
                                if ((!settings.codePareto || codeScore >= maxCodeScore) &&
                                    (!settings.dataPareto || dataScore >= maxDataScore) &&
                                    (!settings.syscallsPareto || syscallScore >= maxSyscallScore) &&
                                    (!settings.controlFlowPareto || codeFlowScore >= maxCodeFlowScore) &&
                                    (!settings.dataFlowPareto || dataFlowScore >= maxDataFlowScore) &&
                                    (!settings.frequencyPareto || frequencyScore >= maxFrequencyScore) &&
                                    (!settings.orderPareto || orderScore >= maxOrderScore))
                                {
                                    maxCodeScore = codeScore;
                                    maxDataScore = dataScore;
                                    maxSyscallScore = syscallScore;
                                    maxCodeFlowScore = codeFlowScore;
                                    maxDataFlowScore = dataFlowScore;
                                    maxFrequencyScore = frequencyScore;
                                    maxOrderScore = orderScore;
                                    best = j;
                                }
                            }
                            else
                            {
                                throw new Exception("Unsupported");
                            }
                        }
                        if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST && best != null)
                            AddGuessedMapping(i, best);
                    }
                }
            }
        }

        public void InsExtendMapping(Settings settings)
        {
            foreach (Instruction i in program1.Values)
            {
                // Don't search for more matches then maxMatches
                if (i.mapping.Count >= settings.maxMatches)
                    continue;

                // Initialise the max variables
                Double maxCodeScore = 0;
                Double maxDataScore = 0;
                Double maxSyscallScore = 0;
                Double maxCodeFlowScore = 0;
                Double maxDataFlowScore = 0;
                Double maxFrequencyScore = 0;
                Double maxOrderScore = 0;

                // Used to keep the best match for 1 iteration of 'i'.
                // Used with WhenToMatchOrUnmatch.BESTORWORST
                Instruction best = null;

                foreach (Instruction j in program2.Values)
                {
                    // Again, do not add more matches then maxMatches
                    if (j.mapping.Count >= settings.maxMatches)
                        continue;
                    if (i.mapping.Count >= settings.maxMatches)
                        break;

                    Double codeScore = 0;
                    Double dataScore = 0;
                    Double syscallScore = 0;
                    Double codeFlowScore = 0;
                    Double dataFlowScore = 0;
                    Double frequencyScore = 0;
                    Double orderScore = 0;

                    if (settings.order)
                    {
                        orderScore = CompareInsByOrder(i, j);
                        if (orderScore < settings.orderThreshold)
                            continue;
                    }
                    if (settings.frequency)
                    {
                        frequencyScore = CompareInsByFrequency(i, j);
                        if (frequencyScore < settings.execThreshold)
                            continue;
                    }
                    if (settings.code)
                    {
                        codeScore = CompareInsBySyntax(i, j);
                        if (codeScore < settings.insThreshold)
                            continue;
                    }
                    if (settings.data)
                    {
                        dataScore = CompareInsByDataOrMinusOne(i, j);
                        if (dataScore == -1 || dataScore < settings.dataThreshold)
                            continue;
                    }
                    if (settings.controlFlow)
                    {
                        codeFlowScore = CompareInsByControlFlow(i, j, settings.cfDirection, settings.cfgLevel);
                        if (codeFlowScore < settings.cfgThreshold)
                            continue;
                    }
                    if (settings.dataFlow)
                    {
                        dataFlowScore = CompareInsByDataFlowOrMinusOne(i, j, settings.dfDirection, settings.dfgLevel);
                        if (dataFlowScore == -1 || dataFlowScore < settings.dfgThreshold)
                            continue;
                    }
                    if (settings.syscalls)
                    {
                        syscallScore = CompareInsBySyscallsOrMinusOne(i, j);
                        if (syscallScore == -1 || syscallScore < settings.syscallThreshold)
                            continue;
                    }

                    // If control flow reaches this point, the score for this instruction-pair
                    // is higher then all selected treshholds!
                    // All: add all good matches
                    if (settings.whenToMatch == WhenToMatchOrUnmatch.ALL)
                        AddGuessedMapping(i, j);
                    // Best or worst: only add the best match.
                    else if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST)
                    {
                        if ((!settings.codePareto || codeScore >= maxCodeScore) &&
                                    (!settings.dataPareto || dataScore >= maxDataScore) &&
                                    (!settings.syscallsPareto || syscallScore >= maxSyscallScore) &&
                                    (!settings.controlFlowPareto || codeFlowScore >= maxCodeFlowScore) &&
                                    (!settings.dataFlowPareto || dataFlowScore >= maxDataFlowScore) &&
                                    (!settings.frequencyPareto || frequencyScore >= maxFrequencyScore) &&
                                    (!settings.orderPareto || orderScore >= maxOrderScore))
                        {
                            maxCodeScore = codeScore;
                            maxDataScore = dataScore;
                            maxSyscallScore = syscallScore;
                            maxCodeFlowScore = codeFlowScore;
                            maxDataFlowScore = dataFlowScore;
                            maxFrequencyScore = frequencyScore;
                            maxOrderScore = orderScore;
                            best = j;
                        }
                    }
                    else
                    {
                        throw new Exception("Unsupported");
                    }
                }
                // Add the best match
                if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST && best != null)
                    AddGuessedMapping(i, best);
            }
        }

        public void BblExtendMapping(Settings settings)
        {
            foreach (BasicBlock b in program1Cfg.blocks.Values)
            {
                // Do not add more matches then maxMatches
                if (b.mapping.Count >= settings.maxMatches)
                    continue;
                // When the syscalls setting is set, only
                // process bbl's with syscalls
                if (settings.syscalls)
                {
                    bool containsSyscall = false;
                    foreach (Instruction i in b.instructions)
                        if (program1Syscalls.ContainsKey(i))
                            containsSyscall = true;
                    if (!containsSyscall)
                        continue;
                }

                Double maxCodeScore = 0;
                Double maxDataScore = 0;
                Double maxSyscallScore = 0;
                Double maxCodeFlowScore = 0;
                Double maxDataFlowScore = 0;
                Double maxFrequencyScore = 0;
                Double maxOrderScore = 0;

                BasicBlock best = null;

                foreach (BasicBlock c in program2Cfg.blocks.Values)
                {
                    // Check if the maxMatches limit isn't reached yet
                    if (c.mapping.Count >= settings.maxMatches)
                        continue;
                    if (b.mapping.Count >= settings.maxMatches)
                        break;
                    // If the syscall setting is set, skip bbl's
                    // without a syscall
                    if (settings.syscalls)
                    {
                        bool containsSyscall = false;
                        foreach (Instruction i in c.instructions)
                            if (program2Syscalls.ContainsKey(i))
                                containsSyscall = true;
                        if (!containsSyscall)
                            continue;
                    }

                    // Reset the scores
                    Double codeScore = 0;
                    Double dataScore = 0;
                    Double syscallScore = 0;
                    Double codeFlowScore = 0;
                    Double dataFlowScore = 0;
                    Double frequencyScore = 0;
                    Double orderScore = 0;

                    if (settings.order)
                    {
                        orderScore = CompareBblByOrder(b, c);
                        if (orderScore < settings.orderThreshold)
                            continue;
                    }
                    if (settings.frequency)
                    {
                        frequencyScore = CompareBblByFrequency(b, c);
                        if (frequencyScore < settings.execThreshold)
                            continue;
                    }
                    if (settings.code)
                    {
                        codeScore = CompareBblBySyntax(b, c);
                        if (codeScore < settings.insThreshold)
                            continue;
                    }
                    if (settings.data)
                    {
                        dataScore = CompareBblByDataOrMinusOne(b, c);
                        if (dataScore == -1 || dataScore < settings.dataThreshold)
                            continue;
                    }
                    if (settings.controlFlow)
                    {
                        codeFlowScore = CompareBblByControlFlow(b, c, settings.cfDirection, settings.cfgLevel);
                        if (codeFlowScore < settings.cfgThreshold)
                            continue;
                    }
                    if (settings.dataFlow)
                    {
                        dataFlowScore = CompareBblByDataFlowOrMinusOne(b, c, settings.dfDirection, settings.dfgLevel);
                        if (dataFlowScore == -1 || dataFlowScore < settings.dfgThreshold)
                            continue;
                    }
                    if (settings.syscalls)
                    {
                        syscallScore = CompareBblBySyscallsOrMinusOne(b, c);
                        if (syscallScore == -1 || syscallScore < settings.syscallThreshold)
                            continue;
                    }


                    // The control flow only reaches this point if the scores were above the
                    // tresholds for all the selected classifiers.
                    if (settings.whenToMatch == WhenToMatchOrUnmatch.ALL)
                        MatchInstructionsOfBasicBlocks(b, c);
                    // Update the max scores and 'best' variable when this match
                    // is better then any other so far.
                    else if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST)
                    {
                        if ((!settings.codePareto || codeScore >= maxCodeScore) &&
                                    (!settings.dataPareto || dataScore >= maxDataScore) &&
                                    (!settings.syscallsPareto || syscallScore >= maxSyscallScore) &&
                                    (!settings.controlFlowPareto || codeFlowScore >= maxCodeFlowScore) &&
                                    (!settings.dataFlowPareto || dataFlowScore >= maxDataFlowScore) &&
                                    (!settings.frequencyPareto || frequencyScore >= maxFrequencyScore) &&
                                    (!settings.orderPareto || orderScore >= maxOrderScore))
                        {
                            maxCodeScore = codeScore;
                            maxDataScore = dataScore;
                            maxSyscallScore = syscallScore;
                            maxCodeFlowScore = codeFlowScore;
                            maxDataFlowScore = dataFlowScore;
                            maxFrequencyScore = frequencyScore;
                            maxOrderScore = orderScore;
                            best = c;
                        }
                    }
                    else
                    {
                        throw new Exception("Unsupported");
                    }
                }
                // When best or worst is the whenToMatch setting, add the match in the
                // 'best' variable to the matches.
                if (settings.whenToMatch == WhenToMatchOrUnmatch.BESTORWORST && best != null)
                    MatchInstructionsOfBasicBlocks(b, best);
            }
        }

        public void DoMapping(Settings settings)
        {
            if (settings.granularity == Granularity.BASICBLOCK)
            {
                if (settings.phase == Phase.INIT)
                    BblInitMapping(settings);
                else if (settings.phase == Phase.EXTEND)
                    BblExtendMapping(settings);
                else if (settings.phase == Phase.FILTER)
                    BblFilterMapping(settings);
            }
            else if (settings.granularity == Granularity.TRANSITION)
            {
                if (settings.phase == Phase.INIT)
                    TransInitMapping(settings);
                else if (settings.phase == Phase.EXTEND)
                    TransExtendMapping(settings);
                else if (settings.phase == Phase.FILTER)
                    TransFilterMapping(settings);
            }
            else if (settings.granularity == Granularity.INSTRUCTION)
            {
                if (settings.phase == Phase.INIT)
                    InsInitMapping(settings);
                else if (settings.phase == Phase.EXTEND)
                    InsExtendMapping(settings);
                else if (settings.phase == Phase.FILTER)
                    InsFilterMapping(settings);
            }
        }

        public void InsInitMapping(Settings settings)
        {
            ClearInsMappings();
            InsExtendMapping(settings);
        }

        public void TransInitMapping(Settings settings)
        {
            ClearInsMappings();
            TransExtendMapping(settings);
        }

        public void BblInitMapping(Settings settings)
        {
            ClearInsMappings();
            ClearBblMappings();
            BblExtendMapping(settings);
        }

        private void ClearBblMappings()
        {
            foreach (BasicBlock b in program1Cfg.blocks.Values)
                b.clearMapping();
            foreach (BasicBlock b in program2Cfg.blocks.Values)
                b.clearMapping();
        }

        private void ClearInsMappings()
        {
            foreach (Instruction i in program1.Values)
                i.clearGuessedMappings();
            foreach (Instruction i in program2.Values)
                i.clearGuessedMappings();
        }
        
        internal Double CompareInsBySyscalls(Instruction b, Instruction c)
        {
            double value = CompareInsBySyscallsOrMinusOne(b, c);
            if (value == -1)
                return 0;
            else return value;
        }
        
        internal Double CompareInsBySyscallsOrMinusOne(Instruction b, Instruction c)
        {
            if (!syscallsRead)
                throw new Exception("error"); // TODO: make this a NoSyscallsReadException()
            List<Syscall> syscalls1 = new List<Syscall>();
            List<Syscall> syscalls2 = new List<Syscall>();

            if (program1Syscalls.ContainsKey(b))
                foreach (Syscall call in program1Syscalls[b])
                    syscalls1.Add(call);


            if (program2Syscalls.ContainsKey(c))
                foreach (Syscall call in program2Syscalls[c])
                    syscalls2.Add(call);

            return CompareSetOfSyscallsOrMinusOne(syscalls1, syscalls2);
        }
        internal Double CompareBblBySyscalls(BasicBlock b, BasicBlock c)
        {
            double value = CompareBblBySyscallsOrMinusOne(b, c);
            if (value == -1)
                return 0;
            else return value;
        }
        internal Double CompareBblBySyscallsOrMinusOne(BasicBlock b, BasicBlock c)
        {
            if (!syscallsRead)
                throw new Exception("error");
            List<Syscall> syscalls1 = new List<Syscall>();
            List<Syscall> syscalls2 = new List<Syscall>();

            foreach (Instruction i in b.instructions)
                if (program1Syscalls.ContainsKey(i))
                    foreach (Syscall call in program1Syscalls[i])
                        syscalls1.Add(call);

            foreach (Instruction i in c.instructions)
                if (program2Syscalls.ContainsKey(i))
                    foreach (Syscall call in program2Syscalls[i])
                        syscalls2.Add(call);

            return CompareSetOfSyscallsOrMinusOne(syscalls1, syscalls2);
        }

        internal Double CompareInsByData(Instruction b, Instruction c)
        {
            double value = CompareInsByDataOrMinusOne(b, c);
            if (value == -1)
                return 0;
            else return value;
        }
        internal Double CompareInsByDataOrMinusOne(Instruction b, Instruction c)
        {
            if (!valuesRead)
                throw new Exception("error");

            return CompareSetOfIntsOrMinusOne(b.values, c.values);
        }
        internal Double CompareBblByData(BasicBlock b, BasicBlock c)
        {
            double value = CompareBblByDataOrMinusOne(b, c);
            if (value == -1)
                return 0;
            else return value;
        }
        internal double CompareBblByDataOrMinusOne(BasicBlock b, BasicBlock c)
        {
            if (!valuesRead)
                throw new Exception("error");

            return CompareSetOfIntsOrMinusOne(b.data, c.data);
        }

        // Checks for obviously invalid addresses
        public static bool IntIsDiscriminativeAndInvariant(UInt32 i)
        {
            if (i == 0 || i == 1 || i == (UInt32)0xffffffff || i == 0x00badbad)
                return false;
            if (i < 0x08000000)
                return true;
            if (i > 0xc0000000)
                return true;
            if (i < 0x09000000)
                return false;
            if (i > 0xb0000000)
                return false;
            return true;
        }

        internal Double CompareInsByControlFlow(Instruction b, Instruction c)
        {
            return CompareInsByControlFlow(b, c, Direction.BOTH, 3);
        }

        internal Double CompareInsByDataFlow(Instruction b, Instruction c)
        {
            double value = CompareInsByDataFlowOrMinusOne(b, c, Direction.BOTH, 3);
            if (value == -1)
                return 0;
            return value;
        }

        internal Double CompareBblByControlFlow(BasicBlock b, BasicBlock c)
        {
            return CompareBblByControlFlow(b, c, Direction.BOTH, 3);
        }

        internal Double CompareBblByDataFlow(BasicBlock b, BasicBlock c)
        {
            double value = CompareBblByDataFlowOrMinusOne(b, c, Direction.BOTH, 3);
            if (value == -1)
                return 0;
            return value;
        }

        internal Double CompareInsByDataFlowOrMinusOne(Instruction b, Instruction c, Direction direction, int level)
        {
            if (!dgRead)
                throw new Exception("error");

            if (direction == Direction.UP)
                return CompareInsByDfgBackwardOrMinusOne(b, c, level);
            else if (direction == Direction.DOWN)
                return CompareInsByDfgForwardOrMinusOne(b, c, level);
            else if (direction == Direction.BOTH)
            {
                Double score1 = CompareInsByDfgBackwardOrMinusOne(b, c, level);
                Double score2 = CompareInsByDfgForwardOrMinusOne(b, c, level);

                if (score1 == -1 || score2 == -1)
                {
                    if (score1 == -1 && score2 == -1)
                        return -1;
                    else return Math.Max(score1, score2) / 2;
                }
                return (score1 + score2) / 2;
            }
            else throw new Exception(); // TODO: Throw a more verbose exception
        }

        internal Double CompareBblByDataFlowOrMinusOne(BasicBlock bbl1, BasicBlock bbl2, Direction direction, int level)
        {
            if (!dgRead)
                throw new Exception("error");

            if (direction == Direction.UP)
                return CompareBblByDfgBackwardOrMinusOne(bbl1, bbl2, level);
            else if (direction == Direction.DOWN)
                return CompareBblByDfgForwardOrMinusOne(bbl1, bbl2, level);
            else if (direction == Direction.BOTH)
            {
                Double score1 = CompareBblByDfgBackwardOrMinusOne(bbl1, bbl2, level);
                Double score2 = CompareBblByDfgForwardOrMinusOne(bbl1, bbl2, level);

                if (score1 == -1 || score2 == -1)
                {
                    if (score1 == -1 && score2 == -1)
                        return -1;
                    else return Math.Max(score1, score2) / 2;
                }
                return (score1 + score2) / 2;
            }
            else throw new Exception();
        }

        internal Double CompareInsByControlFlow(Instruction b, Instruction c, Direction direction, int level)
        {
            if (direction == Direction.UP)
                return CompareInsByCfgBackward(b, c, level);
            if (direction == Direction.DOWN)
                return CompareInsByCfgForward(b, c, level);
            if (direction == Direction.BOTH)
                return CompareInsByCfgBoth(b, c, level);
            throw new Exception("common");
        }

        internal Double CompareBblByControlFlow(BasicBlock bbl1, BasicBlock bbl2, Direction direction, int level)
        {
            if (direction == Direction.UP)
                return CompareBblByCfgBackward(bbl1, bbl2, level);
            if (direction == Direction.DOWN)
                return CompareBblByCfgForward(bbl1, bbl2, level);
            if (direction == Direction.BOTH)
                return CompareBblByCfgBoth(bbl1, bbl2, level);
            throw new Exception("common");
        }

        internal Double CompareInsByDfgBackwardOrMinusOne(Instruction b, Instruction c, int level)
        {
            System.Collections.Generic.List<Instruction> toCheck1 = GetInsByDfgBackward(b, level);
            System.Collections.Generic.List<Instruction> toCheck2 = GetInsByDfgBackward(c, level);

            //if (toCheck1.Count == 0 || toCheck2.Count == 0)
            //  return 0;

            return CompareSetOfInstructionsOrMinusOne(toCheck1, toCheck2);
        }

        private System.Collections.Generic.List<Instruction> GetInsByDfgBackward(Instruction b, int level)
        {
            System.Collections.Generic.List<Instruction> toCheck1 = new List<Instruction>();


            foreach (Instruction i in b.backwardDependencies)
            {
                AddInstructionsBackwardDcg(toCheck1, i, level);
            }
            return toCheck1;
        }

        internal Double CompareBblByDfgBackwardOrMinusOne(BasicBlock bbl1, BasicBlock bbl2, int level)
        {
            System.Collections.Generic.List<Instruction> toCheck1 = new List<Instruction>();
            System.Collections.Generic.List<Instruction> toCheck2 = new List<Instruction>();

            foreach (Instruction b in bbl1.instructions)
            {
                foreach (Instruction i in b.backwardDependencies)
                {
                    AddInstructionsBackwardDcg(toCheck1, i, level);
                }
            }

            foreach (Instruction c in bbl2.instructions)
            {
                foreach (Instruction i in c.backwardDependencies)
                {
                    AddInstructionsBackwardDcg(toCheck2, i, level);
                }
            }

            return CompareSetOfBasicBlocksOrMinusOne(SetOfInstructionsToSetOfBasicBlocks(toCheck1), SetOfInstructionsToSetOfBasicBlocks(toCheck2));
        }

        private List<BasicBlock> SetOfInstructionsToSetOfBasicBlocks(List<Instruction> toCheck1)
        {
            List<BasicBlock> Bbls = new List<BasicBlock>();
            foreach (Instruction i in toCheck1)
            {
                if (!Bbls.Contains(i.basicBlock))
                    Bbls.Add(i.basicBlock);
            }
            return Bbls;
        }

        internal Double CompareInsByDfgForwardOrMinusOne(Instruction b, Instruction c, int level)
        {
            System.Collections.Generic.List<Instruction> toCheck1 = new List<Instruction>();
            System.Collections.Generic.List<Instruction> toCheck2 = new List<Instruction>();

            foreach (Instruction i in b.forwardDependencies)
            {
                AddInstructionsForwardDcg(toCheck1, i, level);
            }
            foreach (Instruction i in c.forwardDependencies)
            {
                AddInstructionsForwardDcg(toCheck2, i, level);
            }
            //if (toCheck1.Count == 0 || toCheck2.Count == 0)
            //    return 0;
            return CompareSetOfInstructionsOrMinusOne(toCheck1, toCheck2);
        }

        internal Double CompareBblByDfgForwardOrMinusOne(BasicBlock bbl1, BasicBlock bbl2, int level)
        {
            System.Collections.Generic.List<Instruction> toCheck1 = new List<Instruction>();
            System.Collections.Generic.List<Instruction> toCheck2 = new List<Instruction>();
            foreach (Instruction b in bbl1.instructions)
            {
                foreach (Instruction i in b.forwardDependencies)
                {
                    AddInstructionsForwardDcg(toCheck1, i, level);
                }
            }
            foreach (Instruction c in bbl2.instructions)
            {
                foreach (Instruction i in c.forwardDependencies)
                {
                    AddInstructionsForwardDcg(toCheck2, i, level);
                }
            }

            return CompareSetOfBasicBlocksOrMinusOne(SetOfInstructionsToSetOfBasicBlocks(toCheck1), SetOfInstructionsToSetOfBasicBlocks(toCheck2));
        }
        /*
        internal Double CompareBblByCfgBackward(BasicBlock bbl1, BasicBlock bbl2)
        {
            System.Collections.Generic.List<Instruction> toCheckBefore1 = new List<Instruction>();
            System.Collections.Generic.List<Instruction> toCheckBefore2 = new List<Instruction>();
            foreach (BasicBlock a in bbl1.predecessors)
            {
                AddBasicBlocksBackward(toCheckBefore1, a, cfgLevel);
            }
            foreach (BasicBlock a in bbl2.predecessors)
            {
                AddBasicBlocksBackward(toCheckBefore2, a, cfgLevel);
            }
            return CompareSetOfInstructions(toCheckBefore1, toCheckBefore2);
        }
        internal Double CompareBblByCfgForward(BasicBlock bbl1, BasicBlock bbl2)
        {
            System.Collections.Generic.List<Instruction> toCheckAfter1 = new List<Instruction>();
            System.Collections.Generic.List<Instruction> toCheckAfter2 = new List<Instruction>();
            foreach (BasicBlock a in bbl1.successors)
            {
                AddBasicBlocksForward(toCheckAfter1, a, cfgLevel);
            }
            foreach (BasicBlock a in bbl2.successors)
            {
                AddBasicBlocksForward(toCheckAfter2, a, cfgLevel);
            }
            return CompareSetOfInstructions(toCheckAfter1, toCheckAfter2);
        }
        */

        //TODO adjust to instruction level
        internal System.Collections.Generic.List<Instruction> GetInstructionsForward(Instruction i, int level)
        {
            System.Collections.Generic.List<Instruction> list = new List<Instruction>();

            /*not first in bbl*/
            if (i.basicBlock.instructions.Find(i).Next != null)
            {
                GoFurtherDown(list, i.basicBlock.instructions.Find(i).Next, level);
            }
            else
            {
                foreach (BasicBlock a in i.basicBlock.successors)
                {
                    GoFurtherDown(list, a.instructions.First, level);
                }
            }
            return list;
        }
        private void GoFurtherUp(List<Instruction> list, LinkedListNode<Instruction> linkedListNode, int level)
        {
            if (!list.Contains(linkedListNode.Value))
            {
                list.Add(linkedListNode.Value);
                level--;
            }
            else return;
            if (level > 0)
            {
                if (linkedListNode.Previous != null)
                    GoFurtherUp(list, linkedListNode.Previous, level);
                else
                {
                    foreach (BasicBlock a in linkedListNode.Value.basicBlock.predecessors)
                    {
                        GoFurtherUp(list, a.instructions.Last, level);
                    }
                }
            }
        }
        private void GoFurtherDown(List<Instruction> list, LinkedListNode<Instruction> linkedListNode, int level)
        {
            if (!list.Contains(linkedListNode.Value))
            {
                list.Add(linkedListNode.Value);
                level--;
            }
            else return;
            if (level > 0)
            {
                if (linkedListNode.Next != null)
                    GoFurtherDown(list, linkedListNode.Next, level);
                else
                {
                    foreach (BasicBlock a in linkedListNode.Value.basicBlock.successors)
                    {
                        GoFurtherDown(list, a.instructions.First, level);
                    }
                }
            }
        }

        internal System.Collections.Generic.List<Instruction> GetInstructionsBackward(Instruction i, int level)
        {
            System.Collections.Generic.List<Instruction> list = new List<Instruction>();

            /*not first in bbl*/
            if (i.basicBlock.instructions.Find(i).Previous != null)
            {
                GoFurtherUp(list, i.basicBlock.instructions.Find(i).Previous, level);
            }
            else
            {
                foreach (BasicBlock a in i.basicBlock.predecessors)
                {
                    GoFurtherUp(list, a.instructions.Last, level);
                }
            }

            return list;
        }

        internal Double CompareInsByCfgBackward(Instruction b, Instruction c, int level)
        {
            double value = CompareSetOfInstructionsOrMinusOne(GetInstructionsBackward(b, level), GetInstructionsBackward(c, level));
            //set to 0 works as well, but it surprises me that this occurs.
            if (value == -1)
                value = 0;// throw new Exception();
            return value;
        }

        internal Double CompareInsByCfgForward(Instruction b, Instruction c, int level)
        {
            double value = CompareSetOfInstructionsOrMinusOne(GetInstructionsForward(b, level), GetInstructionsForward(c, level));
            //set to 0 works as well, but it surprises me that this occurs.
            if (value == -1)
                value = 0;// throw new Exception();
            return value;
        }

        internal Double CompareInsByCfgBoth(Instruction b, Instruction c, int level)
        {
            List<Instruction> list1 = GetInstructionsForward(b, level);
            foreach (Instruction i in GetInstructionsBackward(b, level))
                if (!list1.Contains(i))
                    list1.Add(i);

            List<Instruction> list2 = GetInstructionsForward(c, level);
            foreach (Instruction i in GetInstructionsBackward(c, level))
                if (!list2.Contains(i))
                    list2.Add(i);

            double value = CompareSetOfInstructionsOrMinusOne(list1, list2);
            //set to 0 works as well, but it surprises me that this occurs.
            if (value == -1)
                throw new Exception();
            return value;
        }

        internal Double CompareBblByCfgBackward(BasicBlock bbl1, BasicBlock bbl2, int level)
        {
            System.Collections.Generic.List<BasicBlock> toCheckBefore1 = new List<BasicBlock>();
            System.Collections.Generic.List<BasicBlock> toCheckBefore2 = new List<BasicBlock>();
            foreach (BasicBlock a in bbl1.predecessors)
            {
                AddBasicBlocksBackward(toCheckBefore1, a, level);
            }
            foreach (BasicBlock a in bbl2.predecessors)
            {
                AddBasicBlocksBackward(toCheckBefore2, a, level);
            }
            double value = CompareSetOfBasicBlocksOrMinusOne(toCheckBefore1, toCheckBefore2);
            if (value == -1)
                value = 0;//throw new Exception();
            return value;
        }


        internal Double CompareBblByCfgForward(BasicBlock bbl1, BasicBlock bbl2, int level)
        {
            System.Collections.Generic.List<BasicBlock> toCheckAfter1 = new List<BasicBlock>();
            System.Collections.Generic.List<BasicBlock> toCheckAfter2 = new List<BasicBlock>();
            foreach (BasicBlock a in bbl1.successors)
            {
                AddBasicBlocksForward(toCheckAfter1, a, level);
            }

            foreach (BasicBlock a in bbl2.successors)
            {
                AddBasicBlocksForward(toCheckAfter2, a, level);
            }
            double value = CompareSetOfBasicBlocksOrMinusOne(toCheckAfter1, toCheckAfter2);
            if (value == -1)
                value=0;//throw new Exception();
            return value;
        }

        internal Double CompareBblByCfgBoth(BasicBlock bbl1, BasicBlock bbl2, int level)
        {
            System.Collections.Generic.List<BasicBlock> toCheckAfter1 = new List<BasicBlock>();
            System.Collections.Generic.List<BasicBlock> toCheckAfter2 = new List<BasicBlock>();
            foreach (BasicBlock a in bbl1.successors)
            {
                AddBasicBlocksForward(toCheckAfter1, a, level);
            }
            foreach (BasicBlock a in bbl1.predecessors)
            {
                AddBasicBlocksBackward(toCheckAfter1, a, level);
            }

            foreach (BasicBlock a in bbl2.successors)
            {
                AddBasicBlocksForward(toCheckAfter2, a, level);
            }
            foreach (BasicBlock a in bbl2.predecessors)
            {
                AddBasicBlocksBackward(toCheckAfter2, a, level);
            }
            double value = CompareSetOfBasicBlocksOrMinusOne(toCheckAfter1, toCheckAfter2);
            if (value == -1)
                throw new Exception();
            else return value;
        }

        private double CompareSetOfSyscallsOrMinusOne(List<Syscall> syscalls1, List<Syscall> syscalls2)
        {
            if (syscalls1.Count == 0 || syscalls2.Count == 0)
            {
                if (syscalls1.Count == 0 && syscalls2.Count == 0)
                    return -1;
                return 0;
            }

            double score = 0;
            foreach (Syscall s in syscalls1)
            {
                double best_match = 0;
                foreach (Syscall t in syscalls2)
                {
                    double result = Syscall.GetScore(s, t);
                    if (result > best_match)
                        best_match = result;
                }
                score += best_match;
            }

            foreach (Syscall s in syscalls2)
            {
                double best_match = 0;
                foreach (Syscall t in syscalls1)
                {
                    double result = Syscall.GetScore(s, t);
                    if (result > best_match)
                        best_match = result;
                }
                score += best_match;
            }

            return score / (syscalls1.Count + syscalls2.Count);
        }

        private Double CompareSetOfIntsOrMinusOne(List<UInt32> toCheck1, List<UInt32> toCheck2)
        {
            if (toCheck1.Count == 0 || toCheck2.Count == 0)
            {
                if (toCheck1.Count == 0 && toCheck2.Count == 0)
                    return -1;
                else return 0;
            }

            double score = 0;
            int matching = 0;
            foreach (UInt32 i in toCheck1)
            {
                foreach (UInt32 j in toCheck2)
                {
                    if (i == j)
                    {
                        matching++;
                        break;
                    }
                }
            }
            score += matching * 1.0 / toCheck1.Count;

            matching = 0;
            foreach (Int32 i in toCheck2)
            {
                foreach (Int32 j in toCheck1)
                {
                    if (i == j)
                    {
                        matching++;
                        break;
                    }
                }
            }
            score += matching * 1.0 / toCheck2.Count;
            return score / 2;
        }

        private double CompareSetOfBasicBlocksOrMinusOne(List<BasicBlock> toCheckBefore1, List<BasicBlock> toCheckBefore2)
        {
            if (toCheckBefore1.Count == 0 || toCheckBefore2.Count == 0)
            {
                if (toCheckBefore1.Count == 0 && toCheckBefore2.Count == 0)
                    return -1;
                return 0;
            }

            double score = 0;
            int matching = 0;
            foreach (BasicBlock i in toCheckBefore1)
            {
                foreach (BasicBlock j in toCheckBefore2)
                {
                    if (j.ContainsMappingTo(i))
                    {
                        matching++;
                        break;
                    }
                }
            }
            score += matching * 1.0 / toCheckBefore1.Count;

            int oldMatching = matching;
            matching = 0;
            foreach (BasicBlock i in toCheckBefore2)
            {
                foreach (BasicBlock j in toCheckBefore1)
                {
                    if (j.ContainsMappingTo(i))
                    {
                        matching++;
                        break;
                    }
                }
            }
            if(matching!=oldMatching)
                System.Console.WriteLine(" common"); // ???
            score += matching * 1.0 / toCheckBefore2.Count;
            return score / 2;
        }

        private Double CompareSetOfInstructionsOrMinusOne(List<Instruction> toCheckBefore1, List<Instruction> toCheckBefore2)
        {
            if (toCheckBefore1.Count == 0 || toCheckBefore2.Count == 0)
            {
                if (toCheckBefore1.Count == 0 && toCheckBefore2.Count == 0)
                    return -1;
                else return 0;
            }

            double score = 0;
            int matching = 0;
            foreach (Instruction i in toCheckBefore1)
            {
                foreach (Instruction j in toCheckBefore2)
                {
                    if (j.ContainsMappingTo(i))
                    {
                        matching++;
                        break;
                    }
                }
            }
            score += matching * 1.0 / toCheckBefore1.Count;

            matching = 0;
            foreach (Instruction i in toCheckBefore2)
            {
                foreach (Instruction j in toCheckBefore1)
                {
                    if (j.ContainsMappingTo(i))
                    {
                        matching++;
                        break;
                    }
                }
            }

            score += matching * 1.0 / toCheckBefore2.Count;
            return score / 2;
        }
        /*
        private void AddBasicBlocksBackward(List<Instruction> toCheck, BasicBlock b, int p)
        {
            foreach (Instruction i in b.instructions)
            {
                if (!toCheck.Contains(i))
                {
                    toCheck.Add(i);
                    p--;
                }
            }
            if (p > 0)
                foreach (BasicBlock a in b.predecessors)
                    if (!toCheck.Contains(a.instructions.First.Value))
                        AddBasicBlocksBackward(toCheck, a, p);
        }
        private void AddBasicBlocksForward(List<Instruction> toCheck, BasicBlock b, int p)
        {
            foreach (Instruction i in b.instructions)
            {
                if (!toCheck.Contains(i))
                {
                    toCheck.Add(i);
                    p--;
                }
            }
            if (p > 0)
                foreach (BasicBlock a in b.successors)
                    if (!toCheck.Contains(a.instructions.First.Value))
                        AddBasicBlocksForward(toCheck, a, p);
        }
        */



        private void AddBasicBlocksBackward(List<BasicBlock> toCheck, BasicBlock b, int p)
        {
            // p = depth?
            if (!toCheck.Contains(b))
            {
                toCheck.Add(b);
                p--;
            }
            else return;

            if (p > 0)
                foreach (BasicBlock a in b.predecessors)
                    if (!toCheck.Contains(a))
                        AddBasicBlocksBackward(toCheck, a, p);
        }

        private void AddBasicBlocksForward(List<BasicBlock> toCheck, BasicBlock b, int p)
        {
            if (!toCheck.Contains(b))
            {
                toCheck.Add(b);
                p--;
            }
            else return;

            if (p > 0)
                foreach (BasicBlock a in b.successors)
                    if (!toCheck.Contains(a))
                        AddBasicBlocksForward(toCheck, a, p);
        }

        private void AddInstructionsBackwardDcg(List<Instruction> toCheck1, Instruction ins, int dfgLevel)
        {
            if (!toCheck1.Contains(ins))
            {
                toCheck1.Add(ins);
                dfgLevel--;
            }
            else return;

            if (dfgLevel > 0)
                foreach (Instruction i in ins.backwardDependencies)
                    if (!toCheck1.Contains(i))
                        AddInstructionsBackwardDcg(toCheck1, i, dfgLevel);
        }

        private void AddInstructionsForwardDcg(List<Instruction> toCheck1, Instruction ins, int dfgLevel)
        {
            if (!toCheck1.Contains(ins))
            {
                toCheck1.Add(ins);
                dfgLevel--;
            }
            else return;

            if (dfgLevel > 0)
                foreach (Instruction i in ins.forwardDependencies)
                    if (!toCheck1.Contains(i))
                        AddInstructionsForwardDcg(toCheck1, i, dfgLevel);
        }

        /*
        private void MatchInstructionsOfBasicBlocks(BasicBlock b, BasicBlock c)
        {
            foreach (Instruction i in b.instructions)
            {
                double best_score = 0;
                Instruction best_ins = null;
                foreach (Instruction j in c.instructions)
                {
                    double tmp_score = Instruction.CompareInstructions(i, j);
                    if (tmp_score > best_score)
                    {
                        best_score = tmp_score;
                        best_ins = j;
                    }
                }
                if (best_score == 1)
                    AddGuessedMapping(i, best_ins);
            }
        }*/

        private void UnmatchAllInstructions()
        {
            foreach (Instruction i in program1.Values)
            {
                List<Instruction> list = new List<Instruction>();
                foreach (Instruction j in i.mapping.Values)
                {
                    list.Add(j);
                }
                foreach (Instruction j in list)
                {
                    RemoveGuessedMapping(i, j);
                }
            }
        }
        private void UnmatchInstructionsOfBasicBlocks(BasicBlock b, BasicBlock c)
        {
            b.removeMatch(c);
            c.removeMatch(b);

            foreach (Instruction i in b.instructions)
            {
                List<Instruction> list = new List<Instruction>();
                foreach (Instruction j in i.mapping.Values)
                {
                    list.Add(j);
                }
                foreach (Instruction j in list)
                {
                    if (j.basicBlock == c)
                        RemoveGuessedMapping(i, j);
                }
            }
        }

        bool bblMatch = true;

        private void MatchInstructionsOfBasicBlocks(BasicBlock b, BasicBlock c)
        {
            b.addMatch(c);
            c.addMatch(b);

            if (bblMatch) // Nobody sets bblMatch aparently. So it's always True.
            {
                bool correctBbl = false;
                foreach (Instruction i in b.instructions)
                {
                    foreach (Instruction j in i.shouldBeMapped.Values) // TODO: Using shouldBeMapped in matching?? Isn't that cheating?
                    {
                        if (j.basicBlock == c)
                        {
                            AddGuessedMapping(i, j);
                            correctBbl = true;
                        }
                    }
                }
                if (!correctBbl)
                {
                    foreach (Instruction i in b.instructions)
                    {
                        AddGuessedMapping(i, c.instructions.First.Value);
                    }
                }
            }
            else
            {
                foreach (Instruction i in b.instructions)
                {
                    foreach (Instruction j in c.instructions)
                    {
                        if (Instruction.CompareInstructions(i, j) == 1 && !j.ContainsMappingTo(i.basicBlock))
                        {
                            AddGuessedMapping(i, j);
                            break;
                        }
                    }
                }

                foreach (Instruction i in b.instructions)
                {
                    if (i.ContainsMappingTo(c))
                        continue;
                    foreach (Instruction j in c.instructions)
                    {
                        if (!j.ContainsMappingTo(b))
                        {
                            AddGuessedMapping(i, j);
                            break;
                        }
                    }
                }
            }
        }

        //Double[][][] Scores = null;
        /*
        enum ScoreType
        {
            SYSCALLS,
            EXECUTION_COUNTS,
            EXECUTION_ORDER,
            INS_COMPARE,
            COUNT
        }
        private void StartScores()
        {
            Scores = new Double[program1Cfg.blocks.Count][][];
            for (int i = 0; i < program1Cfg.blocks.Count; i++)
                Scores[i] = new Double[program2Cfg.blocks.Count][];
            for (int i = 0; i < program1Cfg.blocks.Count; i++)
                for (int j = 0; j < program2Cfg.blocks.Count; j++)
                    Scores[i][j] = new Double[(int)ScoreType.COUNT];
        }
        private void ScoreSyscalls()
        {
            foreach (Syscall call1 in program1Syscalls)
            {
                foreach (Syscall call2 in program2Syscalls)
                {
                    Scores[program1Cfg.blocks.IndexOfKey(program1[call1.ip].basicBlock.from)][program2Cfg.blocks.IndexOfKey(program2[call2.ip].basicBlock.from)][(int)ScoreType.SYSCALLS] = Syscall.GetScore(call1, call2);
                }
            }
        }
        private void ScoreExecutionCounts()
        {
            int i = 0;
            foreach (BasicBlock b in program1Cfg.blocks.Values)
            {
                int j = 0;
                foreach (BasicBlock c in program2Cfg.blocks.Values)
                {
                    Scores[i][j][(int)ScoreType.EXECUTION_COUNTS] = CompareExecutionCounts(b, c);
                    j++;
                }
                i++;
            }
        }
        private void ScoreExecutionOrder()
        {
            Int32 max1 = 0;
            Int32 max2 = 0;
            foreach (BasicBlock b in program1Cfg.blocks.Values)
            {
                if (b.nr > max1) max1 = b.nr;
            }
            foreach (BasicBlock b in program2Cfg.blocks.Values)
            {
                if (b.nr > max2) max2 = b.nr;
            }
            Double factor = 1.0 + (max2 - max1) * 1.0 / max1;
            foreach (BasicBlock b in program1Cfg.blocks.Values)
                foreach (BasicBlock c in program2Cfg.blocks.Values)
                {
                    Scores[program1Cfg.blocks.IndexOfKey(b.from)][program2Cfg.blocks.IndexOfKey(c.from)][(int)ScoreType.EXECUTION_ORDER] = 1 - (c.nr - b.nr * factor) / max2;
                }
        }
        private void ScoreInstructions()
        {
            int i = 0;
            foreach (BasicBlock b in program1Cfg.blocks.Values)
            {
                int j = 0;
                foreach (BasicBlock c in program2Cfg.blocks.Values)
                {
                    Scores[i][j][(int)ScoreType.INS_COMPARE] = CompareBblByInstructions(b, c);
                    j++;
                }
                i++;
            }
        }
        */

        static double factor = 0;
        static int max2 = 0;
        internal double CompareBblByOrder(BasicBlock b, BasicBlock c)
        {
            if (factor == 0)
            {
                int max1 = 0;
                foreach (BasicBlock a in program1Cfg.blocks.Values)
                {
                    if (a.nr > max1) max1 = a.nr;
                }
                foreach (BasicBlock a in program2Cfg.blocks.Values)
                {
                    if (a.nr > max2) max2 = a.nr;
                }
                factor = 1.0 + (max2 - max1) * 1.0 / max1;
            }

            return 1 - Math.Abs((c.nr - b.nr * factor)) / max2;
        }

        internal double CompareInsByOrder(Instruction b, Instruction c)
        {
            return CompareBblByOrder(b.basicBlock, c.basicBlock);
        }

        internal double CompareInsBySyntax(Instruction b, Instruction c)
        {
            return Instruction.CompareInstructions(b, c);
        }

        internal double CompareBblBySyntax(BasicBlock b, BasicBlock c)
        {
            double score = 0;
            foreach (Instruction i in b.instructions)
            {
                double best_score = 0;
                foreach (Instruction j in c.instructions)
                {
                    double tmp_score = Instruction.CompareInstructions(i, j);
                    if (tmp_score > best_score)
                        best_score = tmp_score;
                }
                score += best_score;
            }
            score /= Math.Max(b.instructions.Count, c.instructions.Count);
            return score;
        }

        public delegate Double ScoreInstructions(Instruction x, Instruction y);
        public delegate Double ScoreBasicBlocks(BasicBlock x, BasicBlock y);

        public Dictionary<Int32, List<BasicBlock>> GetClassCountsBasedOnBasicBlocks(ClassifierType type)
        {
            ScoreBasicBlocks classifier = SetBblClassifier(type);

            Dictionary<BasicBlock, Int32> bblDict = new Dictionary<BasicBlock, int>();
            Dictionary<Int32, List<BasicBlock>> classCounts = new Dictionary<int, List<BasicBlock>>();

            foreach (BasicBlock i in program1Cfg.blocks.Values)
            {
                bblDict[i] = 0;
            }

            int classNumber = 0;

            foreach (BasicBlock i in program1Cfg.blocks.Values)
            {
                int counter = 0;
                if (bblDict[i] != 0)
                    continue;
                classNumber++;

                foreach (BasicBlock j in program1Cfg.blocks.Values)
                {
                    if (bblDict[j] != 0)
                        continue;
                    if (classifier(i, j.mapping[0]) == 1)
                    {
                        bblDict[j] = classNumber;
                        counter++;
                    }
                }
                if (classCounts.ContainsKey(counter))
                    classCounts[counter].Add(i);
                else
                {
                    classCounts[counter] = new List<BasicBlock>();
                    classCounts[counter].Add(i);
                }
            }
            return classCounts;
        }

        public Dictionary<Int32, List<Instruction>> GetClassCountsBasedOnInstructions(ClassifierType type)
        {
            ScoreInstructions classifier = SetInsClassifier(type);

            Dictionary<Instruction, Int32> insDict = new Dictionary<Instruction, int>();
            Dictionary<Int32, List<Instruction>> classCounts = new Dictionary<int, List<Instruction>>();

            foreach (Instruction i in program1.Values)
            {
                insDict[i] = 0;
            }

            int classNumber = 0;

            foreach (Instruction i in program1.Values)
            {
                int counter = 0;
                if (insDict[i] != 0)
                    continue;
                classNumber++;

                foreach (Instruction j in program1.Values)
                {
                    if (insDict[j] != 0)
                        continue;
                    if (classifier(i, j.mapping.Values[0]) == 1)
                    {
                        insDict[j] = classNumber;
                        counter++;
                    }
                }
                if (classCounts.ContainsKey(counter))
                    classCounts[counter].Add(i);
                else
                {
                    classCounts[counter] = new List<Instruction>();
                    classCounts[counter].Add(i);
                }
            }
            return classCounts;
        }

        private ScoreInstructions SetInsClassifier(ClassifierType type)
        {
            ScoreInstructions classifier;
            switch (type)
            {
                case ClassifierType.SYNTAX:
                    classifier = new ScoreInstructions(CompareInsBySyntax);
                    break;
                case ClassifierType.DATA:
                    classifier = new ScoreInstructions(CompareInsByData);
                    break;
                case ClassifierType.CONTROLFLOW:
                    classifier = new ScoreInstructions(CompareInsByControlFlow);
                    break;
                case ClassifierType.DATAFLOW:
                    classifier = new ScoreInstructions(CompareInsByDataFlow);
                    break;
                case ClassifierType.FREQUENCY:
                    classifier = new ScoreInstructions(CompareInsByFrequency);
                    break;
                case ClassifierType.ORDER:
                    classifier = new ScoreInstructions(CompareInsByOrder);
                    break;
                case ClassifierType.SYSCALLS:
                    classifier = new ScoreInstructions(CompareInsBySyscalls);
                    break;
                default:
                    throw new Exception("no good");
            }
            return classifier;
        }

        public Int64[] GetScoresBasedOnInstructions(ClassifierType type)
        {
            ScoreInstructions classifier = SetInsClassifier(type);

            Int64[] OkAndWrongAt = new Int64[202];
            foreach (Instruction i in program1.Values)
            {
                foreach (Instruction j in program2.Values)
                {
                    if (i.shouldBeMapped.ContainsValue(j))
                    {
                        int index = (Int32)(classifier(i, j) * 100);
                        OkAndWrongAt[(Int32)(classifier(i, j) * 100)]++;
                    }
                    else
                    {
                        int index = (Int32)(classifier(i, j) * 100) + 101;
                        OkAndWrongAt[(Int32)(classifier(i, j) * 100) + 101]++;
                    }
                }
            }
            return OkAndWrongAt;
        }

        internal Int64[,] GetScoresForInstructionSyntaxAndData()
        {
            Int64[,] result = new Int64[101,202];
            foreach (BasicBlock i in program1Cfg.blocks.Values)
            {
                foreach (BasicBlock j in program2Cfg.blocks.Values)
                {
                    int indexI = (Int32)(CompareBblBySyntax(i, j) * 100);
                    double tmp = CompareBblByDataOrMinusOne(i, j);
                    if (tmp == -1)
                        continue;
                    int indexJ = (Int32)(tmp * 100);
                    if (i.mapping.Contains(j))
                    {
                        result[indexI,indexJ]++;
                    }
                    else
                    {
                        result[indexI,indexJ+101]++;
                    }
                }
            }
            return result;
        }
        public Int64[] GetScoresBasedOnBasicBlocks(ClassifierType type)
        {
            ScoreBasicBlocks classifier = SetBblClassifier(type);            Int64[] OkAndWrongAt = new Int64[202];            int tmp_counter = 0;            foreach (BasicBlock i in program1Cfg.blocks.Values)            {                tmp_counter++;                foreach (BasicBlock j in program2Cfg.blocks.Values)                {                    if (i.mapping.Contains(j))                    {                        int index = (Int32)(classifier(i, j) * 100);                        OkAndWrongAt[(Int32)(classifier(i, j) * 100)]++;                    }                    else                    {                        int index = (Int32)(classifier(i, j) * 100) + 101;                        OkAndWrongAt[(Int32)(classifier(i, j) * 100) + 101]++;                    }                }            }            return OkAndWrongAt;        }

        private ScoreBasicBlocks SetBblClassifier(ClassifierType type)
        {
            ScoreBasicBlocks classifier;
            switch (type)
            {
                case ClassifierType.SYNTAX:
                    classifier = new ScoreBasicBlocks(CompareBblBySyntax);
                    break;
                case ClassifierType.DATA:
                    classifier = new ScoreBasicBlocks(CompareBblByData);
                    break;
                case ClassifierType.CONTROLFLOW:
                    classifier = new ScoreBasicBlocks(CompareBblByControlFlow);
                    break;
                case ClassifierType.DATAFLOW:
                    classifier = new ScoreBasicBlocks(CompareBblByDataFlow);
                    break;
                case ClassifierType.FREQUENCY:
                    classifier = new ScoreBasicBlocks(CompareBblByFrequency);
                    break;
                case ClassifierType.ORDER:
                    classifier = new ScoreBasicBlocks(CompareBblByOrder);
                    break;
                case ClassifierType.SYSCALLS:
                    classifier = new ScoreBasicBlocks(CompareBblBySyscalls);
                    break;
                default:
                    throw new Exception("no good");
            }
            return classifier;
        }

        internal double CompareInsByFrequency(Instruction b, Instruction c)
        {
            return CompareBblByFrequency(b.basicBlock, c.basicBlock);
        }

        internal double CompareBblByFrequency(BasicBlock b, BasicBlock c)
        {
            Int32 countB = b.execution_count;
            Int32 countC = c.execution_count;

            return 1 - Math.Abs(countB - countC) * 1.0 / Math.Max(countB, countC);
            /*
            if (countB == countC)
                return 1;
            else if (Math.Abs(countC - countB) < Math.Max(2, Math.Max(countB, countC) * .1))
                return 0.8;
            else if (Math.Abs(countC - countB) < Math.Max(4, Math.Max(countB, countC) * .2))
                return 0.7;
            else if (Math.Abs(countC - countB) < Math.Max(6, Math.Max(countB, countC) * .3))
                return 0.6;
            else if (Math.Abs(countC - countB) < Math.Max(8, Math.Max(countB, countC) * .4))
                return 0.6;
            else if (Math.Abs(countC - countB) < Math.Max(10, Math.Max(countB, countC) * .5))
                return 0.5;
            else return 0;*/
        }
        /*
        internal System.Collections.Generic.List<Instruction> getFriends(Instruction ins)
        {
            List<Instruction> ret = new List<Instruction>();
            if (Scores == null)
                return ret;
            BasicBlock b = ins.basicBlock;
            for (int i = 0; i < program2Cfg.blocks.Count; i++)
                if (Scores[b.indexInCfg][i][(int)ScoreType.INS_COMPARE] > 0.8)
                    ret.Add(program2Cfg.blocks.Values[i].instructions.First.Value);
            return ret;
        }*/

        public void printScore()
        {
            Console.WriteLine("OK: " + this.NrOk + "   (" + ((Double)(100 * (this.NrOk * 1.0 / (this.NrOk + this.NrFn)))).ToString("###.##") + "%)");
            Console.WriteLine("FN: " + this.NrFn + "   (" + ((Double)(100 * (this.NrFn * 1.0 / (this.NrOk + this.NrFn)))).ToString("###.##") + "%)");
            Console.WriteLine("FP: " + this.NrFp + "   (" + ((Double)(100 * (this.NrFp * 1.0 / (this.NrOk + this.NrFn)))).ToString("###.##") + "%)");
            Console.WriteLine("Goal: " + (this.NrOk + this.NrFn));
        }

        internal void FakeMatches()
        {
            foreach (Instruction i in program1.Values)
            {
                foreach (Instruction j in i.shouldBeMapped.Values)
                {
                    AddGuessedMapping(i, j);
                    i.basicBlock.addMatch(j.basicBlock);
                    j.basicBlock.addMatch(i.basicBlock);
                }
            }
        }
    }

    static class BblMarker
    {
        private static System.Collections.Generic.SortedList<Int32, BasicBlock> marked;
        public static void Start()
        {
            marked = new SortedList<int, BasicBlock>();
        }
        public static void Mark(BasicBlock b)
        {
            marked.Add(b.from, b);
        }
        public static bool Contains(BasicBlock b)
        {
            return marked.ContainsKey(b.from);
        }
    }

    static class InstructionMarker
    {
        private static System.Collections.Generic.SortedList<Int32, Instruction> marked;
        public static void Start()
        {
            marked = new SortedList<int, Instruction>();
        }
        public static void Mark(Instruction b)
        {
            marked.Add(b.address, b);
        }
        public static bool Contains(Instruction b)
        {
            return marked.ContainsKey(b.address);
        }
    }

    public enum OperandType
    {
        mem,
        reg,
        imm,
        none
    }

    public class Operand
    {
        public OperandType type;
        public UInt32 immediate;
        public UInt32 baseReg;
        public UInt32 indexReg;
        public Int32 scale;

        public Operand()
        {
            type = OperandType.none;
        }

        internal static double CompareOperands(Operand operand, Operand operand_2)
        {
            double score = 0;

            if (operand.type != operand_2.type)
                return 0;

            if (operand.type == OperandType.none)
                return 1;
            else if (operand.type == OperandType.imm)
            {
                if (operand.immediate == operand_2.immediate)
                    return 1;
            }
            else if (operand.type == OperandType.reg)
            {
                if (operand.baseReg == operand_2.baseReg)
                    return 1;
            }
            else if (operand.type == OperandType.mem)
            {
                if (operand.immediate == operand_2.immediate)
                    score += 1.0 / 3;
                if (operand.baseReg == operand_2.baseReg)
                    score += 1.0 / 3;
                if (operand.indexReg == operand_2.indexReg)
                    score += 1.0 / 3;
                //if (operand.scale == operand_2.scale)
                //    score += 0.25;
            }
            return score;
        }
    }

    public class ControlFlowGraph
    {
        public System.Collections.Generic.SortedList<Int32, BasicBlock> blocks;
        public ControlFlowGraph()
        {
            blocks = new SortedList<int, BasicBlock>();
        }

        public void AddInstructions(System.Collections.Generic.SortedList<Int32, Instruction> instructions)
            {
            IEnumerator<KeyValuePair<Int32, BasicBlock>> enumerator_blocks = blocks.GetEnumerator();
            IEnumerator<KeyValuePair<Int32, Instruction>> enumerator_instructions = instructions.GetEnumerator();
            enumerator_instructions.MoveNext();
            enumerator_blocks.MoveNext();

            BasicBlock current = enumerator_blocks.Current.Value;

            while (enumerator_blocks.MoveNext())
            {
                BasicBlock next = enumerator_blocks.Current.Value;
                while (enumerator_instructions.Current.Value.address < next.from)
                {
                    System.Diagnostics.Debug.Assert((enumerator_instructions.Current.Value.address >= current.from));
                    current.instructions.AddLast(enumerator_instructions.Current.Value);
                    enumerator_instructions.Current.Value.basicBlock = current;
                    if (!enumerator_instructions.MoveNext())
                        break;
                }
                current = next;
            }

            do
            {
                current.instructions.AddLast(enumerator_instructions.Current.Value);
                enumerator_instructions.Current.Value.basicBlock = current;
            } while (enumerator_instructions.MoveNext());
        }
    }

    public class BasicBlock
    {
        public LinkedList<Instruction> instructions;
        public Int32 from;
        public Int32 to;
        public Int32 nr;
        public Int32 execution_count;
        public Int32 indexInCfg;

        public List<BasicBlock> successors;
        public List<BasicBlock> predecessors;
        public List<BasicBlock> mapping;

        // Used as a Set
        private List<UInt32> Data;

        public List<UInt32> data
        {
            get
            {
                if (Data != null)
                    return Data;
                Data = new List<uint>();
                foreach (Instruction ins in instructions)
                {
                    foreach (UInt32 i in ins.values)
                    {
                        if (!Data.Contains(i))
                            Data.Add(i);
                    }
                }
                return Data;
            }
        }

        public System.Collections.Generic.Dictionary<Int32, BasicBlock> friends;

        public BasicBlock(int from, int nr, int execution_count)
        {
            this.from = from;
            //this.to = to;
            this.nr = nr;
            this.execution_count = execution_count;
            this.instructions = new LinkedList<Instruction>();
            this.successors = new List<BasicBlock>();
            this.predecessors = new List<BasicBlock>();
            this.friends = new Dictionary<int, BasicBlock>();
            this.mapping = new List<BasicBlock>();
            this.Data = null;
        }

        internal bool isMatched()
        {
            foreach (Instruction i in instructions)
            {
                if (i.mapped)
                    return true;
            }
            return false;
        }

        internal void addMatch(BasicBlock b)
        {
            if (!mapping.Contains(b))
                mapping.Add(b);
        }
        
        internal void removeMatch(BasicBlock b)
        {
            mapping.Remove(b);
        }

        internal void clearMapping()
        {
            mapping = new List<BasicBlock>();
        }

        internal bool ContainsMappingTo(BasicBlock i)
        {
            return mapping.Contains(i);
        }
    }

    public class Instruction
    {
        private string assembly;
        public string opcode;

        private Operand source1;
        public Operand Source1
        {
            get { return source1; }
            set
            {
                source1 = value;
                if (value.type == OperandType.imm && Comparison.IntIsDiscriminativeAndInvariant(value.immediate))
                    values.Add(value.immediate);
            }
        }
        private Operand source2;
        public Operand Source2
        {
            get { return source2; }
            set
            {
                source2 = value;
                if (value.type == OperandType.imm && Comparison.IntIsDiscriminativeAndInvariant(value.immediate))
                    values.Add(value.immediate);
            }
        }

        public Operand dest;
        public int index;
        public BasicBlock basicBlock;

        internal List<Int32> oldAddresses;
        // Guessed mapping
        public SortedList<Int32, Instruction> mapping;
        // Real (required) mapping
        public SortedList<Int32, Instruction> shouldBeMapped;
        public List<Instruction> backwardDependencies;
        public List<Instruction> forwardDependencies;
        public List<UInt32> values;


        private int NrOk;
        private int NrFp;
        private int NrFn;

        public bool mapped
        {
            get { return mapping.Count != 0; }
        }

        public bool correct
        {
            get { return (nrFn == 0 && nrFp == 0); }
        }

        public int nrOk
        {
            get { return NrOk; }
        }

        public int nrFp
        {
            get { return NrFp; }
        }

        public int nrFn
        {
            get { return NrFn; }
        }

        public Int32 address;

        public void UpdateNrs()
        {
            NrOk = 0;
            NrFp = 0;
            NrFn = 0;

            foreach (Instruction i in mapping.Values)
            {
                if (shouldBeMapped.ContainsKey(i.address))
                    NrOk++;
                else NrFp++;
            }
            NrFn = shouldBeMapped.Count - nrOk;
        }

        public Instruction(string assembly, Int32 address, string opcode)
        {
            this.assembly = assembly;
            this.address = address;
            this.mapping = new SortedList<int, Instruction>();
            this.shouldBeMapped = new SortedList<int, Instruction>();
            this.oldAddresses = new List<int>();
            this.backwardDependencies = new List<Instruction>();
            this.forwardDependencies = new List<Instruction>();
            this.opcode = opcode;
            //this.oldValues = new List<RegisterSetValues>();
            this.values = new List<UInt32>();
        }

        public void clearMappings()
        {
            this.mapping = new SortedList<int, Instruction>();
            this.shouldBeMapped = new SortedList<int, Instruction>();
            UpdateNrs();
        }

        public void clearGuessedMappings()
        {
            this.mapping = new SortedList<int, Instruction>();
            UpdateNrs();
        }

        public override string ToString()
        {
            string s = "0x";
            s += address.ToString("x");
            s += " (" + nrOk + "," + nrFp + "," + NrFn + ")";
            s += " ";
            s += assembly;
            return s;
        }

        internal void addRequiredMapping(Instruction instruction)
        {
            if (!this.shouldBeMapped.ContainsKey(instruction.address))
                this.shouldBeMapped.Add(instruction.address, instruction);
            // Note: this seems very inefficiënt...
            UpdateNrs();
        }

        internal void addGuessedMapping(Instruction instruction)
        {
            if (!this.mapping.ContainsKey(instruction.address))
            {
                this.mapping.Add(instruction.address, instruction);
                UpdateNrs();
            }
        }

        internal List<Instruction> getOk()
        {
            List<Instruction> list = new List<Instruction>();
            foreach (KeyValuePair<Int32, Instruction> pair in mapping)
            {
                if (shouldBeMapped.ContainsKey(pair.Key))
                    list.Add(pair.Value);
            }
            return list;
        }

        internal List<Instruction> getFalseNegatives()
        {
            List<Instruction> list = new List<Instruction>();
            foreach (KeyValuePair<Int32, Instruction> pair in shouldBeMapped)
            {
                if (!mapping.ContainsKey(pair.Key))
            list.Add(pair.Value);
            }
            return list;
        }

        internal List<Instruction> getFalsePositives()
        {
            List<Instruction> list = new List<Instruction>();
            foreach (KeyValuePair<Int32, Instruction> pair in mapping)
            {
                if (!shouldBeMapped.ContainsKey(pair.Key))
                    list.Add(pair.Value);
            }
            return list;
        }

        // Move to InstructionComparer?
        internal static double CompareInstructions(Instruction i, Instruction j)
        {
            double score = 0;
            int count = 1;

            // Note: this is where (un)limitinstructionset does some damage
            if (i.opcode == j.opcode)
                score += 1;

            // Limitation will do some damage here too: operands are often changed in the
            // limitation process
            if (i.source1.type != OperandType.none || j.source1.type != OperandType.none)
            {
                score += Operand.CompareOperands(i.source1, j.source1);
                count++;
            }
            if (i.source2.type != OperandType.none || j.source2.type != OperandType.none)
            {
                score += Operand.CompareOperands(i.source2, j.source2);
                count++;
            }
            if (i.dest.type != OperandType.none || j.dest.type != OperandType.none)
            {
                score += Operand.CompareOperands(i.dest, j.dest);
                count++;
            }
            score /= count;
            return score;
        }

        internal bool ContainsMappingTo(BasicBlock basicBlock)
        {
            foreach (KeyValuePair<Int32, Instruction> pair in mapping)
            {
                if (pair.Value.basicBlock == basicBlock)
                    return true;
            }
            return false;
        }

        internal bool ContainsMappingTo(Instruction j)
        {
                return mapping.ContainsKey(j.address);
        }
                
        internal void removeGuessedMapping(Instruction instruction)
                {
            mapping.Remove(instruction.address);
                UpdateNrs();
        }
                internal void addValue(RegisterSetValues passedValues)
        {
                AddRegisterValueOfOperand(passedValues, source1);
            AddRegisterValueOfOperand(passedValues, source2);
            AddRegisterValueOfOperand(passedValues, dest);
                }
                    
        private void AddRegisterValueOfOperand(RegisterSetValues passedValues, Operand operand)
                        {
            if (operand.type == OperandType.reg)
                    {
                            UInt32 toAdd = 8;
                if (((UInt32)operand.baseReg & 0x1) != 0)
                {
                    toAdd = 0;
                }
                else if (((UInt32)operand.baseReg & 0x2) != 0)
                                    {
                    toAdd = 1;
                }
                else if (((UInt32)operand.baseReg & 0x4) != 0)
                {
                    toAdd = 2;
                }
                else if (((UInt32)operand.baseReg & 0x8) != 0)
                {
                    toAdd = 3;
                }
                else if (((UInt32)operand.baseReg & 0x10) != 0)
                {
                    toAdd = 4;
                }
                else if (((UInt32)operand.baseReg & 0x20) != 0)
                {
                    toAdd = 5;
                }
                else if (((UInt32)operand.baseReg & 0x40) != 0)
                {
                    toAdd = 6;
                }
                else if (((UInt32)operand.baseReg & 0x80) != 0)
                {
                    toAdd = 7;
                }

                if (toAdd != 8 && Comparison.IntIsDiscriminativeAndInvariant(passedValues.registers[toAdd]) && !values.Contains(passedValues.registers[toAdd]))
                {
                    values.Add(passedValues.registers[toAdd]);
                }
            }
        }
    }

    public class InstructionComparer : IComparer<Instruction>
    {
        public int Compare(Instruction x, Instruction y)
        {
            if (x.address == y.address)
                return 0;
            else if (x.address < y.address)
                return -1;
            else return 1;
        }
    }

    public class RegisterSetValues
    {
        public UInt32[] registers;
        public RegisterSetValues()
        {
            registers = new UInt32[8];
        }
    }

    public class Syscall
    {
        public static int nrOfRegs = 8;
        public int ip;
        public UInt32[] registers;
        public System.Collections.Generic.Stack<Int32> callStack;
        public Syscall()
        {
            registers = new UInt32[Syscall.nrOfRegs];
            callStack = new Stack<int>();
        }

        internal static double GetScore(Syscall call1, Syscall call2)
        {
            int i = 0;
            double score = 0;
            if (call1.registers[0] != call2.registers[0])
                return 0;

            for (i = 0; i < Syscall.nrOfRegs; i++)
                if (call1.registers[i] == call2.registers[i])
                    score += (1.0 / 8);

            return score;
        }
    }
}
