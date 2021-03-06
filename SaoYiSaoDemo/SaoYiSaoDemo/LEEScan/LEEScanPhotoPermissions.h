//
//  LEEScanPhotoPermissions.h
//  SaoYiSaoDemo
//
//  Created by 李江波 on 2017/12/5.
//  Copyright © 2017年 ljb48229. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface LEEScanPhotoPermissions : NSObject

/**
 相机权限
 */
+ (BOOL)cameraPemission;

/**
请求相机
*/
+ (void)requestCameraPemissionWithResult:(void(^)(BOOL granted))completion;

+ (BOOL)photoPermission;

@end
