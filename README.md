# YGsoftware

## 项目概述
一个基于 Direct3D 11 的轻量级 CAD 原型系统，使用 C++ 和 Win32 构建。 

项目不是成为完整的 CAD 软件，而是用尽可能少的代码，把一个 CAD 系统该有的骨架——绘图、选择、Undo/Redo、图层、文件序列化，方便学习。
 

## 功能

- **直线绘制**：按 `L` 激活工具，左键连续点击绘制折线段，右键结束当前段，空格从上一个端点继续，ESC 退出
- **选择与高亮**：鼠标悬停高亮，左键点选，Ctrl+左键多选，Delete 删除选中实体
- **Undo / Redo**：Ctrl+Z / Ctrl+Y，基于命令模式，支持添加和删除的完整撤销
- **视口导航**：鼠标中键拖拽平移，滚轮以鼠标为中心缩放，自适应动态网格
- **文件序列化**：Ctrl+S 保存为 `.mcad`（二进制格式），Ctrl+O 打开文件对话框加载，支持几何数据、颜色、图层属性的完整读写
- **图层管理**：图层数据结构完整（名称、颜色、可见性、锁定），序列化已接入
  
 

## 构建

```bash
git clone https://github.com/
 ```

**用 Visual Studio 2022 及以上版本打开**  `CMakeLists.txt`。
  
## vcpkg 安装依赖

- 获取 vcpkg 包管理器

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

## 工程目录

```
YGsoftware/
│
├── source/
│   │
│   └── App/          # 
│
├── thirdParty/                    # 第三方依赖
│   │
│   └──imgui/                      # imgui
│
└── CMakeLists.txt

```

## 交流
