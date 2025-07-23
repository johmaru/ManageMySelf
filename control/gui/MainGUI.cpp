//
// Created by Johma_sub on 23/07/2025.
//

#include "MainGUI.h"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

void MainGUI::first_page_navigate(QWidget* window,const GlobalSettings &settings)
{
    if (!window)
    {
        return;
    }

    window->resize(settings.windowSize);
    window->setWindowTitle("Manage my self");

    auto *mainLayout = new QVBoxLayout();
    auto *button = new QPushButton(QApplication::tr("押す"));

    mainLayout->addStretch();

    auto *bottomLayout = new QHBoxLayout();

    bottomLayout->addWidget(button);
    bottomLayout->addStretch();

    mainLayout->addLayout(bottomLayout);

    window->setLayout(mainLayout);

    window->show();
}
