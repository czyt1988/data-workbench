#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QElapsedTimer>
#include <functional>

#include "DAPandas.h"
#include "pybind11/embed.h"

int main(int argc, char* argv[])
{
    Py_SetPythonHome(L"C:\Python37");  //设置python的路径
    try {
        pybind11::scoped_interpreter python_interpreter;
        pybind11::print("hello world");
        //通过csv读取一个dataframe
        DAPyDataFrame df = DAPandas::getInstance().read_csv(QStringLiteral("IBM.csv"));
        if (!df) {
            qDebug().noquote() << DAPandas::getInstance().getLastErrorString();
        }
        //打印加载的dataframe
        pybind11::print(df.object());
        // test columns
        QList< QString > cols = df.columns();
        qDebug() << "columns=" << cols;
        // test empty
        qDebug() << "empty=" << df.empty();
        // test shape
        std::size_t row = df.shape().first;
        std::size_t col = df.shape().second;
        qDebug() << "shape=(" << row << "," << col << ")";
        // test size
        qDebug() << "size=" << df.size();
        // test iat
        //获取series
        //按行打印
        qDebug() << " print row,by iloc";
        for (std::size_t i = 0; i < row; ++i) {

            qDebug() << df.iloc(i);
        }
        qDebug() << " print column,by []";
        for (const QString& c : cols) {
            qDebug() << df[ c ];
        }
        system("pause");
    } catch (const std::exception& e) {
        qDebug() << e.what();
    }
    return 0;
}
