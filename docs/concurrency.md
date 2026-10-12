# Concurrency Model

The feed producer owns each queue's `try_push` side. One consumer owns the `try_pop` side. A queue is never accessed by more than one producer or one consumer.

`SpscQueue` is bounded and nonblocking. A full queue rejects the item and increments its overflow counter. `close()` prevents new items but permits the consumer to drain items already published before close. Queue statistics are observational and may represent a momentarily changing state while producer and consumer threads are active.

Market-data and strategy processing must not wait on TCP observer queues. Each observer gets its own queue and its own asynchronous writer.