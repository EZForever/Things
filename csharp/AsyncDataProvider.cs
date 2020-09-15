using System;
using System.Threading.Tasks;
using System.Collections.Generic;

namespace PEDollController.Threads
{

    // Idea from: https://stackoverflow.com/a/54443866

    class AsyncDataProvider<T>
    {

        private IEnumerator<Task<T>> getResultEnumerator;

        private static IEnumerator<Task<T>> LoopEnumerator(Func<T> func)
        {
            while (true)
            {
                Task<T> task = new Task<T>(func);
                task.Start();
                yield return task;
            }
        }

        public AsyncDataProvider(Func<T> getResult)
        {
            getResultEnumerator = LoopEnumerator(getResult);
            getResultEnumerator.MoveNext();
        }

        public Task<T> Get()
        {
            if (getResultEnumerator.Current.IsCompleted)
                getResultEnumerator.MoveNext();

            return getResultEnumerator.Current;
        }

    }

}
