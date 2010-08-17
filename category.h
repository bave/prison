#ifndef __PRISON_CATEGORY_H_
#define __PRISON_CATEGORY_H_

#ifdef __MACH__
#import <Foundation/Foundation.h>
#import <CommonCrypto/CommonDigest.h>
#endif

// NSString -----------------------------------------------------------
@interface NSString (FileOpen)
+ (id)stringWithFile:(NSString*)path;
@end
@implementation NSString (FileOpen)
+ (id)stringWithFile:(NSString*)path
{
    NSError* err;
    #ifdef __MACH__
    return [NSString stringWithContentsOfFile:path encoding:NSASCIIStringEncoding error:&err];
    #else
    return [NSString stringWithContentsOfFile:path];
    #endif
}
@end

// --------------------------------------------------------------------


// NSArray ------------------------------------------------------------
@interface NSArray (NSArray_integer)
- (int) icount;
@end

@implementation NSArray (NSArray_integer)
- (int) icount
{
    return (int)[self count];
}
@end
// --------------------------------------------------------------------


// NSData -------------------------------------------------------------
@interface NSData (explist)
+ (id)dataWithPlist:(NSString*)file;
@end

@implementation NSData(explist)
+ (id)dataWithPlist:(NSString*)file
{
    id plist = nil;
    NSString* error = nil;
    NSPropertyListFormat format = NSPropertyListXMLFormat_v1_0;
    @try {
        NSData* file_data = [NSData dataWithContentsOfFile:file];
        if (file_data == nil) return nil;

        if  ([NSPropertyListSerialization
                             propertyList:file_data isValidForFormat:format])
        { 
            plist = [NSPropertyListSerialization
                            propertyListFromData:file_data
                                mutabilityOption:NSPropertyListImmutable
                                          format:&format
                                errorDescription:&error];
        }
        if (error != nil) {
            [error autorelease];
            plist = nil;
        }
    }
    @catch (id err) {
    }
    //NSLog(@"%@\n", plist);
    return plist;
}
@end

#ifdef __MACH__
@interface NSData (exDigest)
- (NSData*)sha1Digest;
- (NSData*)md5Digest;
- (NSString*)hexString;
@end

@implementation NSData(exDigest)
- (NSData*)sha1Digest
{
    unsigned char result[CC_SHA1_DIGEST_LENGTH];
    CC_SHA1([self bytes], [self length], result);
    return [NSData dataWithBytes:result length:CC_SHA1_DIGEST_LENGTH];
}

- (NSData*)md5Digest {
    unsigned char result[CC_MD5_DIGEST_LENGTH];
    CC_MD5([self bytes], [self length], result);
    return [NSData dataWithBytes:result length:CC_MD5_DIGEST_LENGTH];
}

- (NSString*)hexString {
    unsigned int i;
    static const char* hexstr[16] = { "0", "1", "2", "3",
                                      "4", "5", "6", "7",
                                      "8", "9", "a", "b",
                                      "c", "d", "e", "f" };
    const char* dataBuffer = (char*)[self bytes];
    NSMutableString* stringBuffer = nil;
    stringBuffer = [NSMutableString stringWithCapacity:([self length]*2)];
    for (i=0; i<[self length]; i++)
    {
        uint8_t t1, t2;
        t1 = (0x00f0 & (dataBuffer[i])) >> 4;
        t2 =  0x000f & (dataBuffer[i]);
        //printf("%s",hexstr[t1]);
        //printf("%s",hexstr[t2]);
        [stringBuffer appendFormat:@"%s", hexstr[t1]];
        [stringBuffer appendFormat:@"%s", hexstr[t2]];
    }

    //return [[stringBuffer copy] autorelease];
    return [NSString stringWithString:stringBuffer];
}
@end
#endif
// --------------------------------------------------------------------

#endif 

