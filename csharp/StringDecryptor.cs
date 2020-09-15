using System;
using System.IO;
using System.Reflection;

namespace CSharpPlayground
{
    internal sealed class StringDecryptor
    {
        public static string Decrypt(string enc, int key)
        {
            return StringDecryptorInternal.decryptor.Decrypt(enc, key);
        }

        public static string resName()
        {
            /*
            char[] array = "\u001c/+,)".ToCharArray();
            int num = array.Length;
            while ((num -= 1) >= 0)
            {
                array[num] = (char)((int)array[num] ^ 0x7f);
            }
            return new string(array);
            */
            return "cPTSV";
        }

        private delegate string resNameDelegate();

        private sealed class StringDecryptorInternal
        {
            private StringDecryptorInternal()
            {
                /*
                Stream manifestResourceStream = Assembly.GetExecutingAssembly().GetManifestResourceStream(getResourceName());
                if (manifestResourceStream!= null)
                {
                    keyArr = new byte[16];
                    manifestResourceStream.Read(keyArr, 0, keyArr.Length);
                }
                */
                keyArr = new byte[16] { 0x0d, 0x2e, 0x9f, 0x23, 0xda, 0x65, 0xc6, 0x77, 0x41, 0xc1, 0x1e, 0x74, 0xba, 0x9a, 0xc5, 0xd5 };
            }

            public string Decrypt(string enc, int key)
            {
                int num = enc.Length;
                char[] array = enc.ToCharArray();
                while ((num -= 1) >= 0)
                {
                    array[num] = (char)((int)array[num] ^ ((int)keyArr[key & 0x0f] | key));
                }
                return new string(array);
            }

            private static readonly resNameDelegate getResourceName = new resNameDelegate(resName);

            public static readonly StringDecryptorInternal decryptor = new StringDecryptorInternal();

            private byte[] keyArr;
        }
    }
}
