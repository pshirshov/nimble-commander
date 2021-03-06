// Copyright (C) 2017 Michael Kazakov. Subject to GNU General Public License version 3.
#include "RefreshPanel.h"
#include "../PanelController.h"

namespace nc::panel::actions {
    
void RefreshPanel::Perform( PanelController *_target, id _sender ) const
{
    [_target forceRefreshPanel];
}

}
