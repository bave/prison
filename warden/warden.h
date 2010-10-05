#import <Cocoa/Cocoa.h>


@interface warden : NSObject {


}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView;

- (id)tableView:(NSTableView *)aTableView
                objectValueForTableColumn:(NSTableColumn *)aTableColumn
                row:(NSInteger)rowIndex;

- (id)init;
- (void)dealloc;

- (IBAction)rcOpen:(id)sender;

@end
