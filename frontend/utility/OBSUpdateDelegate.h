/******************************************************************************
 Copyright (C) 2024 by Patrick Heyer <opensource@patrickheyer.com>
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#pragma once

#import <Foundation/Foundation.h>
#import <Sparkle/Sparkle.h>

#import <QAction>

@interface OBSUpdateDelegate : NSObject <SPUUpdaterDelegate> {
}
@property (copy) NSString *_Nonnull branch;
@property (nonatomic) SPUStandardUpdaterController *_Nonnull updaterController;

- (nonnull NSSet<NSString *> *)allowedChannelsForUpdater:(nonnull SPUUpdater *)updater;
- (void)observeCanCheckForUpdatesWithAction:(nonnull QAction *)action;
- (void)observeValueForKeyPath:(NSString *_Nullable)keyPath
                      ofObject:(id _Nullable)object
                        change:(NSDictionary<NSKeyValueChangeKey, id> *_Nullable)change
                       context:(void *_Nullable)context;

@end
