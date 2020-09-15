using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpPlayground
{
    public static class X19Encryptor
    {
        public static string Encrypt(string str)
        {
            return ENCMAGIC + X19EncryptorInternal.Encrypt(str, ENCKEY);
        }

        public static string Decrypt(string str)
        {
            return str.StartsWith(ENCMAGIC) ? X19EncryptorInternal.Decrypt(str.Remove(0, ENCMAGIC.Length), ENCKEY) : str;
        }

        private const string ENCKEY = "942894570397f6d1c9cca2535ad18a2b";

        private const string ENCMAGIC = "!x19sign!";

        private static class X19EncryptorInternal
        {
            public static string Encrypt(string str, string key)
            {
                return LongToString(Encrypt(ByteToLong(Encoding.UTF8.GetBytes(str.PadRight(32, '\0'))), ByteToLong(Encoding.UTF8.GetBytes(key.PadRight(32, '\0')))));
            }
            
            public static string Decrypt(string str, string key)
            {
                string result;
                if (string.IsNullOrWhiteSpace(str))
                {
                    result = str;
                }
                else
                {
                    byte[] array = LongToByte(Decrypt(StringToLong(str), ByteToLong(Encoding.UTF8.GetBytes(key.PadRight(32, '\0')))));
                    result = Encoding.UTF8.GetString(array, 0, array.Length);
                }
                return result;
            }

            // This is a 64-bit variant of XXTEA algorithm. However all the procedures are exactly the same as original 32-bit version

            private static long[] Encrypt(long[] data, long[] key)
            {
                int n = data.Length;
                long[] result;
                if (n < 1)
                {
                    result = data;
                }
                else
                {
                    long z = data[data.Length - 1];
                    long y = data[0];
                    long sum = 0L;
                    long q = (long)(6 + 52 / n);
                    for (; ; )
                    {
                        long num6 = q;
                        q = num6 - 1L;
                        if (num6 <= 0L)
                        {
                            break;
                        }
                        sum -= 0x9e3779b9;
                        long e = sum >> 2 & 3L;
                        long p;
                        for (p = 0L; p < (long)(n - 1); p += 1L)
                        {
                            y = data[(int)(checked((IntPtr)(unchecked(p + 1L))))];
                            z = (data[(int)(checked((IntPtr)p))] += ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4) ^ (sum ^ y) + (key[(int)(checked((IntPtr)((p & 3L) ^ e)))] ^ z)));
                        }
                        y = data[0];
                        z = (data[n - 1] += ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4) ^ (sum ^ y) + (key[(int)(checked((IntPtr)((p & 3L) ^ e)))] ^ z)));
                    }
                    result = data;
                }
                return result;
            }

            private static long[] Decrypt(long[] data, long[] key)
            {
                int n = data.Length;
                long[] result;
                if (n < 1)
                {
                    result = data;
                }
                else
                {
                    long z = data[data.Length - 1];
                    long y = data[0];
                    long q = (long)(6 + 52 / n);
                    for (long sum = q * 0x9e3779b9; sum != 0L; sum -= 0x9e3779b9)
                    {
                        long e = sum >> 2 & 3L;
                        long p;
                        for (p = (long)(n - 1); p > 0L; p -= 1L)
                        {
                            z = data[(int)(checked((IntPtr)(unchecked(p - 1L))))];
                            y = (data[(int)(checked((IntPtr)p))] -= ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4) ^ (sum ^ y) + (key[(int)(checked((IntPtr)((p & 3L) ^ e)))] ^ z)));
                        }
                        z = data[n - 1];
                        y = (data[0] -= ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4) ^ (sum ^ y) + (key[(int)(checked((IntPtr)((p & 3L) ^ e)))] ^ z)));
                    }
                    result = data;
                }
                return result;
            }

            private static long[] ByteToLong(byte[] data)
            {
                int num = ((data.Length % 8 == 0) ? 0 : 1) + data.Length / 8;
                long[] array = new long[num];
                for (int i = 0; i < num - 1; i++)
                {
                    array[i] = BitConverter.ToInt64(data, i * 8);
                }
                byte[] array2 = new byte[8];
                Array.Copy(data, (num - 1) * 8, array2, 0, data.Length - (num - 1) * 8);
                array[num - 1] = BitConverter.ToInt64(array2, 0);
                return array;
            }

            private static byte[] LongToByte(long[] data)
            {
                List<byte> list = new List<byte>(data.Length * 8);
                for (int i = 0; i < data.Length; i++)
                {
                    list.AddRange(BitConverter.GetBytes(data[i]));
                }
                while (list[list.Count - 1] == 0)
                {
                    list.RemoveAt(list.Count - 1);
                }
                return list.ToArray();
            }

            private static long[] StringToLong(string str)
            {
                int num = str.Length / 16;
                long[] array = new long[num];
                for (int i = 0; i < num; i++)
                {
                    array[i] = Convert.ToInt64(str.Substring(i * 16, 16), 16);
                }
                return array;
            }

            private static string LongToString(long[] data)
            {
                StringBuilder stringBuilder = new StringBuilder();
                for (int i = 0; i < data.Length; i++)
                {
                    stringBuilder.Append(data[i].ToString("x2").PadLeft(16, '0'));
                }
                return stringBuilder.ToString();
            }
        }

    }
}
