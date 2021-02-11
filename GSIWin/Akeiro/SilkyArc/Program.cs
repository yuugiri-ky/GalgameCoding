using System;

namespace SilkyArc
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine("Usage: SilkyArc path\\to\\folder output.arc");
                return;
            }

            Packer packer = new Packer();
            packer.PackFolder(args[0], args[1]);
        }
    }
}
