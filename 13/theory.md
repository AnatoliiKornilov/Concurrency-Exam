# 13. Примитивы `Future` и `Promise`. Применение для асинхронного выполнения задач. `Callback hell`. `Future continuations`, `chaining` и `combining`.


## `Future` и `Promise`
+ `Future` — представление результата, который станет доступен в будущем: можно проверить готовность, синхронно дождаться (`wait`), забрать значение (`get`).
+ `Promise` — «продюсер»: устанавливает значение (`set_value`) или исключение (`set_exception`) в общее состояние (`shared state`).

## Задача future/base

Реализуйте базовый функционал `Future`. Важным отличием от реализации `std-future` является наличие метода `Subscribe`, который позволяет "подписываться" на результат выполнения асинхронной операции:
```cpp
FooFuture(input).Subscribe([](Result<int>&& result) noexcept {
    std::cerr << "FooFuture has completed with value: " << *result << std::endl;
});
```
Данный метод скорее предназначен для реализации других методов композиции, чем для непосредственного использования, однако сделаем его публичным для простоты.

## Задача future/chaining. Последовательная композиция

Давайте попробуем решить следующую задачу. Пусть есть потенциально долгие операции a, b, c и d и нам необходимо последовательно передавать результаты операций друг за другом: `a -> b -> c -> d`. Одним из простых, но неэффективных способов решения данной задачи является написание кода, который будет блокироваться в ожидании завершения операции. Другой подход заключается в использовании колбэков: когда асинхронная операция завершается, она вызывает колбек. Однако при использовании подхода с колбэками вы можете столкнуться с так называемым "Callback Hell" — или, в языке с поддержкой анонимных функций, возможно, с "Callback Pyramid":
```cpp
void AsyncA(Output, Callback<OutputA>);
void AsyncB(OutputA, Callback<OutputB>);
void AsyncC(OutputB, Callback<OutputC>);
void AsyncD(OutputC, Callback<OutputD>);

auto result = std::make_shared<int>();
FooAsync(input, [=](Output output) {
  // ...
  AsyncA(output, [=](OutputA outputA) {
    // ...
    AsyncB(outputA, [=](OutputB outputB) {
      // ...
      AsyncC(outputB, [=](OutputC outputC) {
        // ...
        AsyncD(outputC, [=](OutputD outputD) {
          *result = outputD * 2;
        });
      });
    });
  });
});

```

С использованием `futures`, последовательно соединённых с помощью `ThenValue`, код становится намного чище:
```cpp
Future<OutputA> FutureA(Output);
Future<OutputB> FutureB(OutputA);
Future<OutputC> FutureC(OutputB);

Future<int> f =
  FooFuture(input)
  .ThenValue(AsyncA)
  .ThenValue(AsyncB)
  .ThenValue(AsyncC)
  .ThenValue([](OutputD outputD) {
    return outputD * 2;
  });
```

Вам необходимо реализовать методы для последовательной композиции `Future`: `ThenValue` и `ThenError`. Каждый из методов принимает функцию, которая может вернуть как `Future`, так и сам результат. Важно, что `ThenValue` может возвращать `Future` с другим `ValueType`.

## Задача future/combining. Параллельная композиция

Рассмотрим теперь пример, в котором небходимо выполнить a и b параллельно и дождаться их завершения. В подходе на основе колбэков задачу можно было бы решить следующим образом:
```cpp
void MultiFooAsync(Callback<OutputA, OutputB> callback, InputA input_a, InputB input_b)
{
  struct Context {
    std::tuple<OutputA, OutputB> outputs;
    std::mutex lock;
    size_t remaining;
  };
  auto context = std::make_shared<Context>();
  context->remaining = 2;

  AsyncA(input_a, [=](OutputA output) {
      std::lock_guard guard(context->lock);
      std::get<0>(context->outputs) = output;
      if (--context->remaining == 0) {
        callback(context->outputs);
      }
  });

  AsyncB(input_b, [=](OutputB output) {
      std::lock_guard guard(context->lock);
      std::get<1>(context->outputs) = output;
      if (--context->remaining == 0) {
        callback(context->outputs);
      }
  });
}
```

Видно, что в решении на основе колбэков необходимо заботиться о создании общего контекста, обеспечении потокобезопасности, а также контролировать счетчик ссылкок, чтобы знать, когда все операции завершатся. Что еще хуже, хотя это и не очевидно в данном примере, когда граф вычислений становится сложным, следить за путем выполнения становится очень трудно. Разработчикам необходимо построить в уме модель всего состояния сервиса и то, как оно взаимодействует с различными входными данными.

Рассмотрим теперь, как данную задачу можно решить с помощью параллельной композиции `Future`:
```cpp
Future<std::tuple<OutputA, OutputB>> MultiFooFuture(InputA input_a, InputB input_b) {
    return Collect(AsyncA(input_a), AsyncB(input_b));
}
```

`Collect` - один из способов параллельной композиции, который необходимо будет реализовать. Он принимает на вход несколько `Future` и возвращает `Future<tuple<...>>`, которая завершится либо когда все принятые `Future` завершаться, либо когда будет брошено исключение в одной из них.

Реализуйте 4 функции для параллельной композиции: `CollectAll`, `Collect`, `CollectAny` и `CollectAnyWithoutException`. В качестве идеи для решения данной задачи можно взять код `MultiFooAsync`.
