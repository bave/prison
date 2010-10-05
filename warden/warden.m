#import "warden.h"


@implementation warden

/*
Protocol detail is NSTableDataSource.
|---------|---------|
| colmun1 | colmun2 |
|---------|---------|
|         |         | rowIndex1
|---------|---------|
|         |         | rowIndex2
|---------|---------|
|         |         |
*/

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return 7;
}

- (id)tableView:(NSTableView *)aTableView
                objectValueForTableColumn:(NSTableColumn *)aTableColumn
                row:(NSInteger)rowIndex
{
    if([[aTableColumn identifier] isEqual:@"hostname"])
    {
        //column 1
        return [NSString stringWithFormat:@"%d",rowIndex];
    }
    
    else if([[aTableColumn identifier] isEqual:@"signed"])
    {
        return [NSString stringWithFormat:@"YES"];
    }
    
    else
    {
        return @"";
    }
}

- (id)init
{
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
    return;
}

- (IBAction)rcOpen:(id)sender
{
    id pool = [NSAutoreleasePool new];

    NSOpenPanel* openPanel;
    NSArray *fileTypes = [NSArray arrayWithObjects:@"plist", nil];
    openPanel = [NSOpenPanel openPanel];

    int result;
    result = [openPanel runModalForTypes:fileTypes];

    if (result == NSOKButton) {
        //[openPanel directory]
        //[openPanel filename]
    }

    if (result == NSCancelButton) {
    }

    [pool drain];
    return;
}

@end
