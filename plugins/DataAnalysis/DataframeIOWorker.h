#ifndef DATAFRAMEIOWORKER_H
#define DATAFRAMEIOWORKER_H
#include <QObject>

/**
 * @brief 负责处理dataframe的io操作的工作者
 */
class DataframeIOWorker : QObject
{
    Q_OBJECT
public:
    explicit DataframeIOWorker(QObject* par = nullptr);
    ~DataframeIOWorker();
public Q_SLOTS:
    void exportIndividualData
};

#endif  // DATAFRAMEIOWORKER_H
