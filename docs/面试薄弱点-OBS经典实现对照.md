# C++/Qt 面试薄弱点：OBS 经典实现与调试路线

本文把 `D:\projects\模拟面试准备\面试薄弱点攻克清单.md` 映射到当前 OBS 仓库。每个主题都给出：先读什么、关键机制、怎么下断点、面试如何表达。

## 1. 访问权限、继承、多态、虚析构

### 经典实现：`json11::JsonValue`

阅读 [`deps/json11/json11.hpp:207`](../deps/json11/json11.hpp:207) 到 [`deps/json11/json11.hpp:230`](../deps/json11/json11.hpp:230)。

```cpp
class JsonValue {
protected:
    virtual Json::Type type() const = 0;
    virtual void dump(std::string &out) const = 0;
    virtual ~JsonValue() {}
};
```

- `protected` 虚函数只给类层次内部使用，外部通过 `Json` 外壳访问。
- 纯虚函数使 `JsonValue` 成为抽象基类，不同 JSON 类型提供不同实现。
- 虚析构保证通过基类指针或 `shared_ptr<JsonValue>` 销毁时走完整析构链。
- `Json` 中的 `std::shared_ptr<JsonValue> m_ptr` 同时展示了运行时多态和共享所有权。

**调试：**在 `Json::dump()` 下断点，观察静态类型与实际派生类型；查看调用栈是否命中具体 JSON 值类型。临时移除 `override` 或将析构改为非虚，只做编译/析构实验，完成后还原。

**面试结论：**类外只能访问 `public`；派生类可直接访问基类 `public/protected`，不能直接访问 `private`。基类 `private` 仍存在于基类子对象中，只是派生类没有直接访问权限。

## 2. STL 六大组件与实际使用

| 组件 | 职责 | 仓库观察点 |
|---|---|---|
| 容器 | 组织对象 | [`frontend/wizards/AutoConfigTestPage.cpp:52`](../frontend/wizards/AutoConfigTestPage.cpp:52) 的 `vector<ServerInfo>` |
| 算法 | 对范围执行操作 | [`frontend/wizards/AutoConfigTestPage.cpp:201`](../frontend/wizards/AutoConfigTestPage.cpp:201) 的 `find_if`；场景排序见 [`frontend/widgets/OBSBasic_SceneCollections.cpp:71`](../frontend/widgets/OBSBasic_SceneCollections.cpp:71) |
| 迭代器 | 统一访问范围 | `begin/end`、`std::begin/std::end` |
| 仿函数 | 参数化可调用行为 | `find_if`/`sort` 的 lambda 和比较器 |
| 适配器 | 改变已有组件的接口 | `back_inserter`、`move_iterator`、`queue` |
| 分配器 | 抽象内存分配 | `vector` 默认分配器；扩容时观察分配、移动、析构 |

### 一个完整组合

```cpp
std::vector<ServerInfo> selected;
selected.reserve(servers.size());
std::copy_if(servers.begin(), servers.end(),
             std::back_inserter(selected),
             [](const ServerInfo &server) { return server.enabled(); });
```

这里 `vector` 是容器，`copy_if` 是算法，`begin/end` 是迭代器，lambda 是仿函数，`back_inserter` 是适配器，底层内存由分配器管理。

**必须掌握的边界：**

- `reserve` 改变 `capacity`，不改变 `size`。
- `vector` 扩容通常使旧迭代器、指针、引用失效；`list` 节点插入通常不影响其他节点迭代器。
- `sort` 需要随机访问迭代器，`find` 的要求更低。
- lambda 捕获引用时必须确认被引用对象仍然存活。
- 分配器管理原始存储，容器负责元素构造、移动、析构和异常安全。

**调试：**在 `reserve/push_back/find_if/sort` 下断点，观察 `size/capacity/data()`、谓词调用次数和扩容前后的元素地址。对 `std::vector<std::unique_ptr<T>>` 单步练习移动所有权。

## 3. Qt 多线程、UI 卡顿、取消和生命周期

### 通用线程封装

阅读 [`shared/qt/wrappers/qt-wrappers.cpp:206`](../shared/qt/wrappers/qt-wrappers.cpp:206) 到 [`shared/qt/wrappers/qt-wrappers.cpp:238`](../shared/qt/wrappers/qt-wrappers.cpp:238)。

关键时序：

1. `QuickThread::run()` 在后台线程执行 `std::function`。
2. GUI 线程进入事件循环，不被耗时函数阻塞。
3. 后台线程使用 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 投递退出请求。
4. `thread->wait()` 等待后台线程结束，之后局部线程对象才销毁。

### 业务线程样本

阅读 [`frontend/wizards/AutoConfigTestPage.cpp:34`](../frontend/wizards/AutoConfigTestPage.cpp:34) 到 [`frontend/wizards/AutoConfigTestPage.cpp:49`](../frontend/wizards/AutoConfigTestPage.cpp:49)，以及 [`AutoConfigTestPage.cpp:98`](../frontend/wizards/AutoConfigTestPage.cpp:98) 到 [`AutoConfigTestPage.cpp:115`](../frontend/wizards/AutoConfigTestPage.cpp:115)。

页面在 GUI 线程更新文本，`std::thread` 执行测试，后台通过 `invokeMethod` 把进度和错误投递回页面。

**调试：**记录 `QThread::currentThreadId()` 和 `qApp->thread()`；让测试函数延迟数秒后关闭页面，观察 `testThread` 是否先停止/等待。重点检查后台 lambda 捕获的 `this` 是否可能在回调执行前失效。卡顿时区分事件循环耗时、队列堆积、锁等待和 IO 延迟。

## 4. STL 智能指针和 OBS 引用模型

- 页面 UI 独占对象：[`frontend/wizards/AutoConfigVideoPage.hpp`](../frontend/wizards/AutoConfigVideoPage.hpp) 的 `std::unique_ptr<Ui_AutoConfigVideoPage>`。
- JSON 共享多态对象：[`deps/json11/json11.hpp:207`](../deps/json11/json11.hpp:207) 的 `std::shared_ptr<JsonValue>`。
- Qt 弱观察指针：[`frontend/OBSApp.hpp:80`](../frontend/OBSApp.hpp:80) 的 `QPointer<OBSMainWindow>`。

**面试边界：**`unique_ptr` 不可拷贝但可移动；`shared_ptr` 共享所有权；`weak_ptr` 不延长对象生命周期。`QPointer` 不是所有权指针，Qt 对象销毁时会自动变空。

## 5. 崩溃治理和 Dump 定位

阅读 [`libobs/obs-win-crash-handler.c:30`](../libobs/obs-win-crash-handler.c:30) 到 [`libobs/obs-win-crash-handler.c:182`](../libobs/obs-win-crash-handler.c:182)。

- `exception_handler_data` 集中保存异常指针、符号函数、版本、CPU、模块和主线程栈。
- `get_dbghelp_imports()` 动态取得 `StackWalk64`、`SymFromAddrW`、`MiniDumpWriteDump`。
- `init_instruction_data()` 从 `CONTEXT` 提取 PC、帧指针、栈指针，并区分 x86/x64/ARM64。
- `init_sym_info()` 初始化符号选项并刷新模块列表。

**定位顺序：**确认 Dump 完整 → 确认二进制/PDB/Build 匹配 → 查看异常码和故障线程 → 查看全部线程栈及模块版本 → 对照最近变更 → 使用 PageHeap、Application Verifier、ASan 或锁顺序日志 → 建立最小复现并补回归用例。

符号匹配只能证明地址可以被解释，不能证明没有内存破坏；故障线程也不一定是根因线程。

## 6. IPC、消息边界和关闭顺序

阅读 [`shared/ipc-util/ipc-util/pipe-windows.c:63`](../shared/ipc-util/ipc-util/pipe-windows.c:63) 到 [`pipe-windows.c:153`](../shared/ipc-util/ipc-util/pipe-windows.c:153)，以及释放流程 [`pipe-windows.c:214`](../shared/ipc-util/ipc-util/pipe-windows.c:214)。

- `PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE` 保留 named pipe 的消息语义。
- 读取线程把数据追加到动态缓冲区，再调用 `read_callback`。
- 关闭顺序是取消 IO → 设置停止事件 → 等待线程 → 关闭句柄 → 释放缓冲区。

这只能类比 TCP 的工程问题，不能把 named pipe 当成 TCP 字节流实现。TCP 仍需长度字段/校验、半包缓存、超时、心跳、指数退避和单连接重连控制。

## 7. 面试口述模板

基础题：

```text
先给结论 -> 指向一个仓库实现 -> 补一个边界 -> 说一个常见误区
```

项目题：

```text
背景 -> 目标指标 -> 我的职责边界 -> 2-4 个步骤 -> 结果 -> 复盘
```

讲崩溃治理时，可以把 OBS 的客户端采集代码作为类比，但不要声称当前仓库包含你经历中的服务端聚合、PDB 平台和线上指标。

## 8. 六次调试练习

1. `json11`：从 `Json::dump()` 跟到具体派生类，画出析构链。
2. `AutoConfigTestPage`：记录 GUI/后台线程 ID，模拟关闭页面时的生命周期问题。
3. STL：比较 `reserve` 前后的扩容和地址失效，改写一次 `find_if/copy_if`。
4. Qt wrapper：跟踪 `invokeMethod` 的队列投递和 `thread->wait()`。
5. 崩溃处理器：从异常上下文跟到模块名、符号和 Dump API。
6. named pipe：画出连接、读取、回调、取消 IO、等待线程、释放句柄的时序图。
