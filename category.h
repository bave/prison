#ifndef __PRISON_CATEGORY_H_
#define __PRISON_CATEGORY_H_

#ifdef __MACH__
#import <Foundation/Foundation.h>
#import <CommonCrypto/CommonDigest.h>
#endif

// NSDictionary -------------------------------------------------------
@interface NSDictionary (HasMethod)
- (BOOL)haskey:(id)key;
@end
@implementation NSDictionary (HasMethod)
- (BOOL)haskey:(id)key
{
    if ([self objectForKey:key]) {
        return YES;
    } else {
        return NO;
    }
}
@end
// --------------------------------------------------------------------


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

@interface NSString (TrimStrip)
- (NSString*)trim;
- (NSString*)strip;
@end

@implementation NSString (TrimStrip)
- (NSString *)trim
{
        return [self stringByTrimmingCharactersInSet:
                        [NSCharacterSet whitespaceAndNewlineCharacterSet]];
}
- (NSString *)strip
{
        return [self trim];
}
@end

@interface NSString (Str2Hex)
- (NSData*)str2hex;
- (NSData*)strHexadecimal;
@end

@implementation NSString (Str2Hex)

- (NSData*)strHexadecimal
{
    return [self str2hex];
}

- (NSData*)str2hex
{
    const char* stringBuffer = (char*)[self UTF8String];
    id dataBuffer = [NSMutableData dataWithCapacity:([self length]/2)];

    while (1) {

        unsigned char c = 0;
        unsigned char c1 = *stringBuffer;
        stringBuffer++;
        unsigned char c2 = *stringBuffer;
        stringBuffer++;

        if (c1 >= '0' && c1 <= '9') {
            c1 = (c1 - '0') << 4;
        } else if (c1 >= 'A' && c1 <= 'F') {
            c1 = (c1 - 'A' + 10) << 4;
        } else if (c1 >= 'a' && c1 <= 'f') {
            c1 = (c1 - 'a' + 10) << 4;
        } else {
            break;
        }

        if (c2 >= '0' && c2 <= '9') {
            c2 = (c2 - '0');
        } else if (c2 >= 'A' && c2 <= 'F') {
            c2 = (c2 - 'A' + 10);
        } else if (c2 >= 'a' && c2 <= 'f') {
            c2 = (c2 - 'a' + 10);
        } else {
            // cant be used..
            break;
        }

        c = c1 | c2;
        [dataBuffer appendBytes:&c length:1];
    }
    //NSLog(@"result:%@", dataBuffer);
    return [NSData dataWithData:dataBuffer];
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
- (NSString*)hex2str;
@end

@implementation NSData(exDigest)

- (NSString*)hex2str
{
    return [self hexString];
}

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

