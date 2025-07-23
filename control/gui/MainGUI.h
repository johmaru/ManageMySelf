//
// Created by Johma_sub on 23/07/2025.
//

#ifndef MAINGUI_H
#define MAINGUI_H
#include <QWidget>

#include "../../fs/global_settings.h"


class MainGUI {
    public:
        ~MainGUI() = default;

        static void first_page_navigate(QWidget* window,const GlobalSettings &settings);
};



#endif //MAINGUI_H
