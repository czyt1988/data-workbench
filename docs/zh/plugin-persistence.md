# 插件数据持久化方案

本文档详细说明插件如何安全、高效地保存和管理数据文件，包括配置存储、节点数据、缓存管理等。

## 主要功能特性

**特性**

- ✅ **持久化需求场景**：配置存储、节点数据、临时缓存、用户数据等场景分析
- ✅ **数据目录规范**：标准目录结构、获取数据目录的方法
- ✅ **文件命名规范**：配置文件、缓存文件、数据文件、日志文件的命名规则
- ✅ **配置存储方案**：QSettings 和 JSON 两种配置存储方式
- ✅ **安全写入最佳实践**：原子写入模式、版本兼容策略
- ✅ **节点数据持久化**：工作流自动序列化、大数据存储策略
- ✅ **缓存管理**：缓存目录创建、清理策略、大小限制
- ✅ **数据访问示例**：完整的数据管理类实现

---

## 持久化需求场景

| 场景 | 数据类型 | 存储方式 |
|------|----------|----------|
| **配置存储** | 用户设置、插件参数 | QSettings / JSON |
| **节点数据** | 工作流节点参数 | 内置序列化机制 |
| **临时缓存** | 处理中间结果 | 临时文件目录 |
| **用户数据** | 自定义数据文件 | 项目目录 / 用户目录 |

---

## 数据目录规范

### 标准目录结构

```text
[项目目录]/
├── workflow.daw              # 工作流文件
├── data/                     # 数据文件目录
│   └── *.csv, *.xlsx
└── plugins/                  # 插件数据目录
    └── MyPlugin/             # 每个插件独立目录
        ├── config.json       # 插件配置
        ├── cache/            # 缓存目录
        │   └── *.tmp
        └── custom/           # 自定义数据
            └── *.dat

[用户配置目录]/
└── DAWorkbench/
    └── plugins/
        └── MyPlugin/
            └── global_config.json
```

### 获取数据目录

```cpp
#include "DAProjectInterface.h"
#include "DADir.h"  // DA::DADir 路径助手

bool MyPlugin::initialize()
{
    DA::DACoreInterface* core = this->core();

    // 获取项目目录（项目加载后有效）
    // 注意：DAProjectInterface 提供 getProjectDir() 和 getProjectFilePath()，没有 getProjectPath()
    DA::DAProjectInterface* project = core->getProjectInterface();
    QString projectDir = project->getProjectDir();              // 如 D:/project
    QString projectFile = project->getProjectFilePath();        // 如 D:/project/da-project.dapro
    QString pluginDataDir = projectDir + "/plugins/MyPlugin";

    // 获取全局配置目录（始终有效，不存在会自动创建）
    // 用 DA::DADir::getConfigPath()，不是 DA::getUserConfigPath()
    QString globalConfigDir = DA::DADir::getConfigPath() + "/plugins/MyPlugin";

    // 获取临时目录（程序结束时自动删除）
    QString tempDir = DA::DADir::getTempPath() + "/MyPlugin";

    // DADir 的路径获取函数会自动确保目录存在，无需 DA::ensureDirectoryExists

    return true;
}
```

---

## 文件命名规范

### 命名规则

| 文件类型 | 命名规范 | 示例 |
|----------|----------|------|
| 配置文件 | `config.json` / `settings.ini` | `config.json` |
| 缓存文件 | `[prefix]_[timestamp].tmp` | `cache_20240310_153022.tmp` |
| 数据文件 | `[plugin]_[name]_[version].dat` | `myplugin_processed_v1.dat` |
| 日志文件 | `[plugin]_[date].log` | `myplugin_20240310.log` |

### 时间戳格式

```cpp
QString generateTimestampFilename(const QString& prefix)
{
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyyMMdd_HHmmss");
    return QString("%1_%2.tmp").arg(prefix, timestamp);
}
```

---

## 配置存储方案

### 方式一：QSettings

!!! tip "适合简单配置"
    适用于键值对形式的简单配置，系统原生支持。

```cpp
// 保存配置（用 DA::DADir::getConfigPath()，不是 DA::getUserConfigPath()）
void MyPlugin::saveConfig()
{
    QSettings settings(DA::DADir::getConfigPath() + "/plugins/MyPlugin/settings.ini",
                       QSettings::IniFormat);

    settings.beginGroup("General");
    settings.setValue("auto_save", m_autoSave);
    settings.setValue("max_cache_size", m_maxCacheSize);
    settings.endGroup();

    settings.beginGroup("Processing");
    settings.setValue("algorithm", m_algorithm);
    settings.setValue("threshold", m_threshold);
    settings.endGroup();

    settings.sync();
}

// 加载配置
void MyPlugin::loadConfig()
{
    QSettings settings(DA::DADir::getConfigPath() + "/plugins/MyPlugin/settings.ini",
                       QSettings::IniFormat);

    settings.beginGroup("General");
    m_autoSave = settings.value("auto_save", true).toBool();
    m_maxCacheSize = settings.value("max_cache_size", 100).toInt();
    settings.endGroup();

    settings.beginGroup("Processing");
    m_algorithm = settings.value("algorithm", "default").toString();
    m_threshold = settings.value("threshold", 0.5).toDouble();
    settings.endGroup();
}
```

### 方式二：JSON 配置

!!! tip "适合复杂配置"
    适用于嵌套结构、数组的复杂配置。

```cpp
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

// 保存 JSON 配置
void MyPlugin::saveJsonConfig()
{
    QJsonObject root;
    
    // 基本配置
    QJsonObject general;
    general["auto_save"] = m_autoSave;
    general["max_cache_size"] = m_maxCacheSize;
    root["general"] = general;
    
    // 处理配置
    QJsonObject processing;
    processing["algorithm"] = m_algorithm;
    processing["threshold"] = m_threshold;
    
    // 数组配置
    QJsonArray filters;
    for (const QString& filter : m_filters) {
        filters.append(filter);
    }
    processing["filters"] = filters;
    
    root["processing"] = processing;
    
    // 写入文件
    QString configPath = DA::DADir::getConfigPath() + "/plugins/MyPlugin/config.json";
    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

// 加载 JSON 配置
void MyPlugin::loadJsonConfig()
{
    QString configPath = DA::DADir::getConfigPath() + "/plugins/MyPlugin/config.json";
    QFile file(configPath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        // 使用默认配置
        loadDefaultConfig();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    QJsonObject root = doc.object();
    
    // 解析基本配置
    QJsonObject general = root["general"].toObject();
    m_autoSave = general["auto_save"].toBool(true);
    m_maxCacheSize = general["max_cache_size"].toInt(100);
    
    // 解析处理配置
    QJsonObject processing = root["processing"].toObject();
    m_algorithm = processing["algorithm"].toString("default");
    m_threshold = processing["threshold"].toDouble(0.5);
    
    // 解析数组
    QJsonArray filters = processing["filters"].toArray();
    m_filters.clear();
    for (const QJsonValue& val : filters) {
        m_filters.append(val.toString());
    }
}
```

---

## 安全写入最佳实践

### 原子写入模式

!!! important "避免数据丢失"
    使用原子写入确保数据完整性，防止写入过程中崩溃导致数据损坏。

```cpp
bool MyPlugin::safeWriteFile(const QString& filePath, const QByteArray& data)
{
    // 1. 创建临时文件
    QString tempPath = filePath + ".tmp";
    
    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        daCritical << "Failed to create temp file:" << tempPath;
        return false;
    }

    // 2. 写入数据
    qint64 written = tempFile.write(data);
    tempFile.flush();
    tempFile.close();

    if (written != data.size()) {
        daCritical << "Write incomplete:" << written << "/" << data.size();
        QFile::remove(tempPath);
        return false;
    }

    // 3. 替换原文件（原子操作）
    if (QFile::exists(filePath)) {
        if (!QFile::remove(filePath)) {
            daCritical << "Failed to remove old file:" << filePath;
            QFile::remove(tempPath);
            return false;
        }
    }

    if (!QFile::rename(tempPath, filePath)) {
        daCritical << "Failed to rename temp file to" << filePath;
        QFile::remove(tempPath);
        return false;
    }

    daInfo << "Successfully saved:" << filePath;
    return true;
}
```

### 版本兼容策略

```cpp
// 配置文件版本管理
void MyPlugin::loadJsonConfig()
{
    QJsonObject root = loadConfigFile();
    
    // 检查版本
    int version = root["version"].toInt(1);
    
    if (version > CURRENT_CONFIG_VERSION) {
        daWarning << "Config version" << version << "is newer than supported"
                  << CURRENT_CONFIG_VERSION;
        // 尝试兼容加载
        return loadConfigWithCompatibility(root);
    }

    if (version < CURRENT_CONFIG_VERSION) {
        daInfo << "Upgrading config from version" << version << "to"
               << CURRENT_CONFIG_VERSION;
        // 升级配置
        root = upgradeConfig(root, version);
    }
    
    // 正常加载
    parseConfig(root);
}

QJsonObject MyPlugin::upgradeConfig(const QJsonObject& oldConfig, int oldVersion)
{
    QJsonObject newConfig = oldConfig;
    newConfig["version"] = CURRENT_CONFIG_VERSION;
    
    // 版本 1 -> 2：添加新字段
    if (oldVersion < 2) {
        newConfig["new_feature_enabled"] = false;
    }
    
    // 版本 2 -> 3：修改字段名
    if (oldVersion < 3) {
        if (oldConfig.contains("old_field_name")) {
            newConfig["new_field_name"] = oldConfig["old_field_name"];
            newConfig.remove("old_field_name");
        }
    }
    
    return newConfig;
}
```

---

## 节点数据持久化

### 工作流自动序列化

!!! note "内置支持"
    DAWorkBench 自动保存工作流和节点数据，插件无需手动处理。

节点参数通过内置序列化机制保存。当前 Python-first 节点用 `@NodeDef` 声明，`Parameter` 声明的参数由 `DAWorkflowSerializer` 自动序列化，**运行时缓存状态**通过 `serialize_runtime_state()` / `deserialize_runtime_state()` 钩子持久化（见 `plugins/DASystemNodes/AGENTS.md` 第九章）：

```python
class TextViewerNode:
    def __init__(self):
        super().__init__()
        self._display_text = ""

    def execute(self, inputs=None, params=None):
        value = (inputs or {}).get("value")
        self._display_text = str(value) if value is not None else ""
        return True

    def serialize_runtime_state(self) -> dict:
        """保存时调用，返回需要持久化的运行时状态。"""
        return {"display_text": getattr(self, "_display_text", "")}

    def deserialize_runtime_state(self, state: dict) -> None:
        """加载时调用，从 state 恢复运行时状态。"""
        self._display_text = state.get("display_text", "")
```

!!! warning "旧 C++ 节点钩子已废弃"
    旧 C++ `DAAbstractNode` 的 `saveToVariant()` / `loadFromVariant()` 虚函数属于已废弃架构，当前工作流节点统一用 Python `@NodeDef` 模型，不要使用这些 C++ 钩子。仅作历史背景：

```cpp
// 旧 C++ 节点序列化钩子（已废弃，当前代码库不再使用）
class MyWorker : public DA::DAAbstractNode  // 此继承关系在当前代码库无法编译
{
public:
    // 保存节点数据（旧）
    QVariant saveToVariant() const override
    {
        QVariantMap data;
        data["algorithm"] = m_algorithm;
        data["threshold"] = m_threshold;
        return data;
    }

    // 加载节点数据（旧）
    void loadFromVariant(const QVariant& var) override
    {
        QVariantMap data = var.toMap();
        m_algorithm = data["algorithm"].toString();
        m_threshold = data["threshold"].toDouble();
    }
};
```

### 大数据存储

对于大型数据（如 DataFrame），不建议序列化到工作流文件。数据包装类为 `DAData`（`src/DAData/DAData.h`），**不是** `DADataPackage`：

```cpp
bool MyWorker::exec()
{
    // 处理大型数据 —— 用 DAData，不是 DADataPackage
    DA::DAData result = processLargeData(inputData);

    // 不要将大数据存储在节点中
    // 而是存储引用或文件路径
    QString dataPath = generateCacheFilePath();
    saveDataToFile(result, dataPath);

    // 只存储路径引用
    setOutputData("output_data", QVariant::fromValue(dataPath));

    return true;
}
```

---

## 缓存管理

### 缓存目录创建

```cpp
// 用 DA::DADir::getTempPath()，不是 DA::getTempPath()
QString MyPlugin::getCacheDirectory()
{
    QString cacheDir = DA::DADir::getTempPath() + "/MyPlugin/cache";

    if (!QDir(cacheDir).exists()) {
        QDir().mkpath(cacheDir);
    }

    return cacheDir;
}
```

### 缓存清理策略

```cpp
void MyPlugin::cleanupOldCache()
{
    QString cacheDir = getCacheDirectory();
    QDir dir(cacheDir);
    
    // 清理超过 7 天的缓存
    QDateTime threshold = QDateTime::currentDateTime().addDays(-7);
    
    QStringList files = dir.entryList(QDir::Files);
    for (const QString& file : files) {
        QFileInfo info(dir.absoluteFilePath(file));
        if (info.lastModified() < threshold) {
            QFile::remove(info.absoluteFilePath());
            daDebug << "Removed old cache:" << file;
        }
    }
}

// 在插件卸载时清理（finalize 是 aboutToUnload 的替代）
void MyPlugin::finalize()
{
    cleanupOldCache();
}
```

### 缓存大小限制

```cpp
bool MyPlugin::shouldClearCache()
{
    QString cacheDir = getCacheDirectory();
    qint64 totalSize = calculateDirectorySize(cacheDir);
    qint64 maxSize = m_maxCacheSize * 1024 * 1024; // MB to bytes
    
    return totalSize > maxSize;
}

qint64 MyPlugin::calculateDirectorySize(const QString& path)
{
    qint64 size = 0;
    QDir dir(path);
    
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
        size += info.size();
    }
    
    return size;
}
```

---

## 数据访问示例

### 完整的数据管理类

```cpp
class MyPluginDataManager
{
public:
    MyPluginDataManager(const QString& pluginName);
    
    // 配置管理
    bool saveConfig(const QJsonObject& config);
    QJsonObject loadConfig();
    
    // 缓存管理
    QString createCacheFile(const QString& prefix);
    bool writeCacheData(const QString& path, const QByteArray& data);
    QByteArray readCacheData(const QString& path);
    void clearCache();
    
    // 用户数据
    QString getUserDataPath(const QString& name);
    bool saveUserData(const QString& name, const QByteArray& data);
    
private:
    QString m_pluginName;
    QString m_configDir;
    QString m_cacheDir;
    QString m_userDataDir;
};

MyPluginDataManager::MyPluginDataManager(const QString& pluginName)
    : m_pluginName(pluginName)
{
    // 用 DA::DADir::getConfigPath()，不是 DA::getUserConfigPath()
    QString basePath = DA::DADir::getConfigPath() + "/plugins/" + pluginName;

    m_configDir = basePath;
    m_cacheDir = basePath + "/cache";
    m_userDataDir = basePath + "/data";

    // 确保目录存在
    QDir().mkpath(m_configDir);
    QDir().mkpath(m_cacheDir);
    QDir().mkpath(m_userDataDir);
}
```

---

## 最佳实践总结

### 1. 分离配置和数据

- **配置**：存储在全局配置目录，跨项目共享
- **数据**：存储在项目目录，项目特定

### 2. 使用原子写入

所有重要数据文件使用原子写入模式，避免数据损坏。

### 3. 版本管理

配置文件包含版本号，支持升级和兼容。

### 4. 定期清理

设置缓存大小限制和过期清理策略。

### 5. 错误处理

```cpp
bool MyPlugin::saveData(const QString& path, const QByteArray& data)
{
    if (!safeWriteFile(path, data)) {
        daCritical << "Failed to save data to" << path;

        // 尝试备用路径
        QString backupPath = generateBackupPath(path);
        if (safeWriteFile(backupPath, data)) {
            daInfo << "Saved to backup:" << backupPath;
            return true;
        }

        return false;
    }
    return true;
}
```

---

## 下一步

- [:material-puzzle: 功能扩展](./plugin-extension.md) - 界面和功能扩展
- [:material-book: 最佳实践](./best-practices.md) - 更多开发建议
- [:material-help-circle: 常见问题](./faq.md) - 常见问题解答

> 详见 [日志系统文档](./dev-guide/logging.md)。
