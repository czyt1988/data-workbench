#include "DAAppStatusBar.h"
#include "DAStatusBarWidget.h"
#include <QStatusBar>
#include "DAAppUI.h"
#include "AppMainWindow.h"
namespace DA
{
DAAppStatusBar::DAAppStatusBar(DAUIInterface* u) : DAStatusBarInterface(u)
{
    DAAppUI* appui = qobject_cast< DAAppUI* >(u);
    m_app          = qobject_cast< AppMainWindow* >(appui->mainWindow());
    buildStatusBar(m_app);
}

DAAppStatusBar::~DAAppStatusBar()
{
}

void DAAppStatusBar::retranslateUi()
{
}

void DAAppStatusBar::showMessage(const QString& message, int timeout)
{
    m_statusBarWidget->showMessage(message, timeout);
}

void DAAppStatusBar::clearMessage()
{
    m_statusBarWidget->clearMessage();
}

void DAAppStatusBar::showProgressBar()
{
    m_statusBarWidget->showProgressBar();
}

void DAAppStatusBar::hideProgressBar()
{
    m_statusBarWidget->hideProgressBar();
}

void DAAppStatusBar::setProgress(int value)
{
    m_statusBarWidget->setProgress(value);
}

void DAAppStatusBar::setProgressText(const QString& text)
{
    m_statusBarWidget->setProgressText(text);
}

void DAAppStatusBar::clearProgressText()
{
    m_statusBarWidget->clearProgressText();
}

void DAAppStatusBar::setBusy(bool busy)
{
    m_statusBarWidget->setBusy(busy);
}

void DAAppStatusBar::resetProgress()
{
    m_statusBarWidget->resetProgress();
}

bool DAAppStatusBar::isProgressBarVisible() const
{
    return m_statusBarWidget->isProgressBarVisible();
}

void DAAppStatusBar::buildStatusBar(AppMainWindow* mainWindow)
{
    QStatusBar* statusBar = new QStatusBar(mainWindow);
    m_statusBarWidget     = new DAStatusBarWidget(statusBar);
    statusBar->addWidget(m_statusBarWidget, 1);
    mainWindow->setStatusBar(statusBar);
}
}
