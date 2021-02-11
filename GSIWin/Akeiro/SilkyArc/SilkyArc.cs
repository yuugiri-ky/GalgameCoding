using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace SilkyArc
{
    class Packer
    {
        struct EntryInfo
        {
            public string Path;
            public string Name;
            public uint Size;
            public uint CompressSize;
            public uint Offset;
        }

        List<EntryInfo> entryInfos;

        public void PackFolder(string path, string arcfile)
        {
            entryInfos = new List<EntryInfo>();

            foreach (string file in Directory.EnumerateFiles(path))
            {
                EntryInfo info = new EntryInfo();
                info.Path = file;
                info.Name = Path.GetFileName(file);

                entryInfos.Add(info);
            }

            FileStream output = new FileStream(arcfile, FileMode.Create);
            BinaryWriter writer = new BinaryWriter(output);

            // Write dummy header

            writer.Write(0);

            foreach (EntryInfo info in entryInfos)
            {
                writer.Write((byte)0);

                for (int i = 0; i < info.Name.Length; i++)
                {
                    writer.Write((byte)0);
                }

                writer.Write(0);
                writer.Write(0);
                writer.Write(0);
            }

            uint tableSize = (uint)output.Length - sizeof(uint);

            // Write entry data

            for (int i = 0; i < entryInfos.Count; i++)
            {
                EntryInfo info = entryInfos[i];

                Console.WriteLine("Adding {0}", info.Name);

                FileStream input = new FileStream(info.Path, FileMode.Open);
                BinaryReader reader = new BinaryReader(input);

                MemoryStream memory = new MemoryStream();
                LzssStream lzss = new LzssStream(memory, CompressionMode.Compress);

                byte[] buffer = reader.ReadBytes((int)input.Length);
                lzss.Write(buffer);

                info.Size = (uint)input.Length;
                info.CompressSize = (uint)memory.Length;
                info.Offset = (uint)output.Length;
                entryInfos[i] = info;

                input.Close();

                writer.Write(memory.GetBuffer());
            }

            // Write header

            Console.WriteLine("Creating header...");

            output.Seek(0, SeekOrigin.Begin);

            writer.Write(tableSize);

            foreach (EntryInfo info in entryInfos)
            {
                writer.Write((byte)info.Name.Length);

                byte[] nameBytes = Encoding.ASCII.GetBytes(info.Name);

                for (int i = 0; i < nameBytes.Length; i++)
                {
                    nameBytes[i] -= (byte)(nameBytes.Length - i);
                }

                writer.Write(nameBytes);

                writer.Write(SwapUInt32(info.CompressSize));
                writer.Write(SwapUInt32(info.Size));
                writer.Write(SwapUInt32(info.Offset));
            }

            output.Close();
            entryInfos.Clear();

            Console.WriteLine("Finished");
        }

        public static ushort SwapUInt16(ushort n)
        {
            return (ushort)(((n & 0xff) << 8) | ((n >> 8) & 0xff));
        }

        public static uint SwapUInt32(uint n)
        {
            return (uint)(((SwapUInt16((ushort)n) & 0xffff) << 0x10) |
                           (SwapUInt16((ushort)(n >> 0x10)) & 0xffff));
        }
    }
}
