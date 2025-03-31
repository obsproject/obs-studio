/******************************************************************************
 Copyright (C) 2025 by Patrick Heyer <PatTheMav@users.noreply.github.com>
 
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

@import Foundation;

#import "CrashHandler.hpp"

namespace {
    NSURL *getDiagnosticReportsDirectory()
    {
        NSFileManager *fileManager = [NSFileManager defaultManager];

        NSArray<NSURL *> *libraryURLs = [fileManager URLsForDirectory:NSLibraryDirectory inDomains:NSUserDomainMask];

        if (libraryURLs.count == 0) {
            blog(LOG_ERROR, "Unable to get macOS user library directory URL");
            return nil;
        }

        NSURL *diagnosticsReportsURL = [libraryURLs[0] URLByAppendingPathComponent:@"logs/DiagnosticReports"];

        return diagnosticsReportsURL;
    }
}  // namespace

namespace OBS {
    PlatformType CrashHandler::getPlatformType() const
    {
        return PlatformType::macOS;
    }

    std::filesystem::path CrashHandler::findLastCrashLog() const
    {
        std::filesystem::path crashLogDirectoryPath;

        NSURL *diagnosticsReportsURL = getDiagnosticReportsDirectory();

        if (!diagnosticsReportsURL) {
            return crashLogDirectoryPath;
        }

        BOOL (^enumerationErrorHandler)(NSURL *_Nonnull, NSError *_Nonnull) =
            ^BOOL(NSURL *_Nonnull url __unused, NSError *_Nonnull error) {
                blog(LOG_ERROR, "Failed to enumerate diagnostics reports directory: %s",
                     error.localizedDescription.UTF8String);

                return NO;
            };

        NSFileManager *fileManager = [NSFileManager defaultManager];

        NSDirectoryEnumerator *dirEnumerator = [fileManager
                       enumeratorAtURL:diagnosticsReportsURL
            includingPropertiesForKeys:@[NSURLNameKey, NSURLIsDirectoryKey]
                               options:(NSDirectoryEnumerationSkipsHiddenFiles) errorHandler:enumerationErrorHandler];

        NSMutableArray<NSURL *> *reportCandidates = [NSMutableArray array];

        for (NSURL *entry in dirEnumerator) {
            NSString *fileName = nil;
            [entry getResourceValue:&fileName forKey:NSURLNameKey error:nil];

            if ([fileName hasPrefix:@"OBS"] && [fileName hasSuffix:@".ips"]) {
                [reportCandidates addObject:entry];
            }
        }

        NSArray<NSURL *> *sortedCandidates = [reportCandidates
            sortedArrayUsingComparator:^NSComparisonResult(NSURL *_Nonnull obj1, NSURL *_Nonnull obj2) {
                NSDate *creationDateObj1 = nil;
                NSDate *creationDateObj2 = nil;

                [obj1 getResourceValue:&creationDateObj1 forKey:NSURLCreationDateKey error:nil];
                [obj2 getResourceValue:&creationDateObj2 forKey:NSURLCreationDateKey error:nil];

                NSComparisonResult result = [creationDateObj2 compare:creationDateObj1];

                return result;
            }];

        if (sortedCandidates.count > 0) {
            NSURL *lastDiagnosticsReport = sortedCandidates[0];
            crashLogDirectoryPath = std::filesystem::u8path(lastDiagnosticsReport.path.UTF8String);
        }

        return crashLogDirectoryPath;
    }

    std::filesystem::path CrashHandler::getCrashLogDirectory() const
    {
        std::filesystem::path crashLogDirectoryPath;

        NSURL *diagnosticsReportsURL = getDiagnosticReportsDirectory();

        if (diagnosticsReportsURL) {
            crashLogDirectoryPath = std::filesystem::u8path(diagnosticsReportsURL.path.UTF8String);
        }

        return crashLogDirectoryPath;
    }
}  // namespace OBS
