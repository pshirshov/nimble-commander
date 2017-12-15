// Copyright (C) 2014-2017 Michael Kazakov. Subject to GNU General Public License version 3.
#pragma once

@interface SelectionWithMaskPopupViewController : NSViewController<NSPopoverDelegate>

- (instancetype) initForWindow:(NSWindow*)_wnd doesSelect:(bool)_select;

@property (nonatomic) function<void(NSString *mask)> handler;

@end
