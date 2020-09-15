using System;
using System.Linq;
using System.Collections.Generic;

delegate T lambda<T>(T x);
delegate lambda lambda(lambda x);

namespace CSharpLab
{
    class Program
    {
        // https://blog.csdn.net/yuanlin2008/article/details/8627081
        // https://176.122.157.231:3457/wiki/%CE%9B%E6%BC%94%E7%AE%97
        public static readonly lambda identity = x => x;
        public static readonly lambda self_apply = x => x(x);
        public static readonly lambda apply = f => x => f(x);

        public static readonly lambda TRUE = x => y => x;
        public static readonly lambda FALSE = x => y => y;
        public static readonly lambda COND = x => y => b => b(x)(y);

        public static readonly lambda NOT = b => b(FALSE)(TRUE);
        public static readonly lambda AND = x => y => x(y)(FALSE);
        public static readonly lambda OR = x => y => x(TRUE)(y);

        public static readonly lambda succ = n => b => b(FALSE)(n);
        public static readonly lambda zero = identity;
        public static readonly lambda one = succ(zero);
        public static readonly lambda two = succ(one);

        public static readonly lambda iszero = n => n(TRUE);
        public static readonly lambda pred = n => iszero(n)(zero)(n(FALSE));

        public static readonly lambda recursive = f => ((lambda)(x => f(_ => x(x))))(x => f(_ => x(x)));
        public static readonly lambda stepper = next_step => n => COND(_ => zero)(_ => next_step(pred(n)))(iszero(n))(identity);

        static void Main(string[] args)
        {
            recursive(stepper)(two);
            Console.WriteLine(COND(identity)(apply)(TRUE) == identity);
            Console.ReadKey();
        }
    }
}
