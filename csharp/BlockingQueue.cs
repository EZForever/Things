using System;
using System.Threading;
using System.Collections.Generic;

namespace PEDollController.Threads
{

    // BlockingQueue provides blocking version of Enqueue() & Dequeue(), implementing basic thread safety
    // Idea from: https://stackoverflow.com/a/530228

    static class BlockingQueue
    {
        public static void BlockingEnqueue<T>(this Queue<T> queue, T item)
        {
            lock(queue)
            {
                queue.Enqueue(item);
                // If just recovered from empty state, send a pulse to the blocked GetCommand() thread
                if (queue.Count == 1)
                    Monitor.PulseAll(queue);
            }
        }

        public static T BlockingDequeue<T>(this Queue<T> queue)
        {
            lock(queue)
            {
                if (queue.Count == 0)
                    Monitor.Wait(queue);
                return queue.Dequeue();
            }
        }
    }
}
