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
#include "DAUtils.h"

bool MyPlugin::initialize()
{
    DA::DACoreInterface* core = this->core();
    
    // 获取项目目录（项目打开后有效）
    DA::DAProjectInterface* project = core->getProjectInterface();
    QString projectPath = project->getProjectPath();
    QString pluginDataDir = projectPath + "/plugins/MyPlugin";
    
    // 获取全局配置目录（始终有效）
    QString globalConfigDir = DA::getUserConfigPath() + "/plugins/MyPlugin";
    
    // 确保目录存在
    DA::ensureDirectoryExists(pluginDataDir);
    DA::ensureDirectoryExists(globalConfigDir);
    
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
// 保存配置
void MyPlugin::saveConfig()
{
    QSettings settings(DA::getUserConfigPath() + "/plugins/MyPlugin/settings.ini",
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
    QSettings settings(DA::getUserConfigPath() + "/plugins/MyPlugin/settings.ini",
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
    QString configPath = DA::getUserConfigPath() + "/plugins/MyPlugin/config.json";
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
    QString configPath = DA::getUserConfigPath() + "/plugins/MyPlugin/config.json";
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
        DA_LOG_ERROR("Failed to create temp file: {}", tempPath);
        return false;
    }
    
    // 2. 写入数据
    qint64 written = tempFile.write(data);
    tempFile.flush();
    tempFile.close();
    
    if (written != data.size()) {
        DA_LOG_ERROR("Write incomplete: {} / {}", written, data.size());
        QFile::remove(tempPath);
        return false;
    }
    
    // 3. 替换原文件（原子操作）
    if (QFile::exists(filePath)) {
        if (!QFile::remove(filePath)) {
            DA_LOG_ERROR("Failed to remove old file: {}", filePath);
            QFile::remove(tempPath);
            return false;
        }
    }
    
    if (!QFile::rename(tempPath, filePath)) {
        DA_LOG_ERROR("Failed to rename temp file to {}", filePath);
        QFile::remove(tempPath);
        return false;
    }
    
    DA_LOG_INFO("Successfully saved: {}", filePath);
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
        DA_LOG_WARNING("Config version {} is newer than supported {}", 
                       version, CURRENT_CONFIG_VERSION);
        // 尝试兼容加载
        return loadConfigWithCompatibility(root);
    }
    
    if (version < CURRENT_CONFIG_VERSION) {
        DA_LOG_INFO("Upgrading config from version {} to {}", 
                    version, CURRENT_CONFIG_VERSION);
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

节点数据通过 `saveToVariant()` 和 `loadFromVariant()` 序列化：

```cpp
class MyWorker : public DA::DAAbstractNode
{
public:
    // 保存节点数据
    QVariant saveToVariant() const override
    {
        QVariantMap data;
        
        // 基本数据
        data["algorithm"] = m_algorithm;
        data["threshold"] = m_threshold;
        
        // 复杂数据
        if (m_customData.isValid()) {
            data["custom_data"] = DA::serializeCustomData(m_customData);
        }
        
        return data;
    }
    
    // 加载节点数据
    void loadFromVariant(const QVariant& var) override
    {
        QVariantMap data = var.toMap();
        
        m_algorithm = data["algorithm"].toString();
        m_threshold = data["threshold"].toDouble();
        
        if (data.contains("custom_data")) {
            m_customData = DA::deserializeCustomData(data["custom_data"]);
        }
    }
};
```

### 大数据存储

对于大型数据（如 DataFrame），不建议序列化到工作流文件：

```cpp
bool MyWorker::exec()
{
    // 处理大型数据
    DA::DADataPackage result = processLargeData(inputData);
    
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
QString MyPlugin::getCacheDirectory()
{
    QString cacheDir = DA::getTempPath() + "/MyPlugin/cache";
    
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
            DA_LOG_DEBUG("Removed old cache: {}", file);
        }
    }
}

// 在插件卸载时清理
void MyPlugin::aboutToUnload()
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
    QString basePath = DA::getUserConfigPath() + "/plugins/" + pluginName;
    
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
        DA_LOG_ERROR("Failed to save data to {}", path);
        
        // 尝试备用路径
        QString backupPath = generateBackupPath(path);
        if (safeWriteFile(backupPath, data)) {
            DA_LOG_INFO("Saved to backup: {}", backupPath);
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