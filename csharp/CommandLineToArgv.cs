using System;
using System.Text;
using System.Collections.Generic;

namespace CSharpPlayground
{
    public static class CommandLineToArgv
    {
        static Dictionary<char, char> pairingCharacters = new Dictionary<char, char>()
        {
            { '"', '"' },
            { '\'', '\'' },
            { '(', ')' },
            { '[', ']' },
            { '{', '}' }
        };
        
        public static List<string> Invoke(string cmd)
        {
            List<string> ret = new List<string>();
            StringBuilder builder = new StringBuilder();

            Stack<char> pairingStack = new Stack<char>();
            bool isEscaped = false;
            foreach(char c in cmd)
            {
                char stackTop = (pairingStack.Count == 0) ? (char)0 : pairingStack.Peek();
                
                if(stackTop == '"' || stackTop == '\'')
                {
                    // If inside a pair of quotes, consider following rules:
                    // 1. (Escape rule) Character after '\\' is ignored (i.e. has no effect on rule appliance);
                    // 2. No matching rule except quotes may apply
                    if(isEscaped)
                    {
                        isEscaped = false;
                    }
                    else
                    {
                        if (c == stackTop)
                            pairingStack.Pop();
                        else if (c == '\\')
                            isEscaped = true;
                    }
                }
                else
                {
                    // Otherwise, consider standard rules:
                    // 1. (Match rule) If `c` is a left-hand pairing character, push it onto the stack;
                    // 2. (Match rule) If `c` is a right-hand pairing character,
                    //    pop the stack if `c` matches the stack top, fail otherwise
                    // 3. (Separate rule) If `c` is a whitespace character and stack is empty,
                    //    ignore `c` and cut the string to be a piece of argv[]
                    if (pairingCharacters.ContainsKey(c))
                    {
                        pairingStack.Push(c);
                    }
                    else if (pairingCharacters.ContainsValue(c))
                    {
                        if (c == pairingCharacters[stackTop])
                            pairingStack.Pop();
                        else
                            throw new ArgumentException("Parens mismatch");
                    }
                    else if (Char.IsWhiteSpace(c) && pairingStack.Count == 0)
                    {
                        if(builder.Length > 0)
                        {
                            ret.Add(builder.ToString());
                            builder.Clear();
                        }
                        continue; // Ignore `c`
                    }
                }
                builder.Append(c); // Add `c` to current piece
            }

            // Add the leftover piece in builder
            if (builder.Length > 0)
                ret.Add(builder.ToString());

            // For now, if still in paren-matching state or escaped state, the string is incomplete and should fail
            if (isEscaped || pairingStack.Count != 0)
                throw new ArgumentException("Incomplete string");

            return ret;
        }
    }
}
