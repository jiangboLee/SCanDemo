# SCanDemo
## 轻量级扫一扫，二维码
由于项目中需要有扫一扫功能，那是开发周期比较紧张，就直接GitHub找了`ZXingObjC`直接使用。但是发现这个第三方功能太齐全，对于我显得太笨重。根本不需要那么多功能。所以就自己东抄抄西抄抄整理了一份轻量级的demo。
话不多说，有需要的可以下载使用。
现在分析项目整体功能：
![项目目录.png](http://upload-images.jianshu.io/upload_images/2868618-866e176f87dcc834.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
###### 首先是是判断相机权限，假如未决定就请求弹框：
```swift
+ (void)requestCameraPemissionWithResult:(void (^)(BOOL))completion {
    AVAuthorizationStatus permission = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    switch (permission) {
        case AVAuthorizationStatusAuthorized:
            completion(YES);
            break;
        case AVAuthorizationStatusDenied:
        case AVAuthorizationStatusRestricted:
            completion(NO);
        case AVAuthorizationStatusNotDetermined:
        {
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (granted) {
                        completion(YES);
                    } else {
                        completion(NO);
                    }
                });
            }];
        }
        default:
            break;
    }
}
```
###### 判断可以使用照相机后设置相机：
```swift
- (void)initParaWithPreView:(UIView*)videoPreView ObjectType:(NSArray*)objType cropRect:(CGRect)cropRect success:(void(^)(NSArray<NSString*> *array))block {
    
    self.blockScanResult = block;
    self.videoPreView = videoPreView;
    
    _session = [[AVCaptureSession alloc] init];
    AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    NSError *error;
    _deviceInput = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
    if (_deviceInput) {
        
        [_session addInput:_deviceInput];
        
        AVCaptureMetadataOutput *metadataOutput = [[AVCaptureMetadataOutput alloc] init];
        [metadataOutput setMetadataObjectsDelegate:self queue:dispatch_get_main_queue()];
        [_session addOutput:metadataOutput];
        metadataOutput.metadataObjectTypes = objType;
        
        _stillImageOutput = [[AVCaptureStillImageOutput alloc] init];
        NSDictionary *outputSettings = [[NSDictionary alloc] initWithObjectsAndKeys:AVVideoCodecJPEG, AVVideoCodecKey, nil];
        [_stillImageOutput setOutputSettings:outputSettings];
        [_session addOutput:_stillImageOutput];
        
        
        AVCaptureVideoPreviewLayer *previewLayer = [[AVCaptureVideoPreviewLayer alloc]initWithSession:_session];
        previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
        previewLayer.frame = cropRect;
        [videoPreView.layer insertSublayer:previewLayer atIndex:0];
        
    
        //先进行判断是否支持控制对焦,不开启自动对焦功能，很难识别二维码。
        if (_deviceInput.device.isFocusPointOfInterestSupported && [_deviceInput.device isFocusModeSupported:AVCaptureFocusModeAutoFocus]) {
            [_deviceInput.device lockForConfiguration:nil];
            [_deviceInput.device setFocusMode:AVCaptureFocusModeContinuousAutoFocus];
            [_deviceInput.device unlockForConfiguration];
        }
    } else {
        [LEEAlertViewController showWithTitle:@"错误" message:[NSString stringWithFormat:@"%@",error]];
    }
}
```
###### 扫描到结果的代理回调：
```swift
- (void)captureOutput:(AVCaptureOutput *)output didOutputMetadataObjects:(NSArray<__kindof AVMetadataObject *> *)metadataObjects fromConnection:(AVCaptureConnection *)connection {
    
    AVMetadataMachineReadableCodeObject *metadataObject = metadataObjects.firstObject;
    if ([metadataObject.type isEqualToString:AVMetadataObjectTypeQRCode]) {
        [self stopScan];
        self.blockScanResult(@[metadataObject.stringValue]);
    }
    
}
```
###### 开始扫码：
```swift
- (void)startScan {
    if (_session != nil && !_session.isRunning) {
        [_session startRunning];
    }
}
```
###### 结束扫码
```swift
- (void)stopScan {
    if (_session != nil && _session.isRunning) {
        [_session stopRunning];
    }
}
```
###### 好的，现在已经能完成基本的扫描功能了，接下来要开始加点视图和动画了：
```swift
- (void)drawScanRect {
    
    CGSize sizeRetangle = CGSizeMake(self.frame.size.width - self.viewStyle.xScanRetangleOffset * 2, self.frame.size.width - self.viewStyle.xScanRetangleOffset * 2);
    //扫码区域Y轴最小坐标
    CGFloat YMinRetangle = self.frame.size.height / 2.0 - sizeRetangle.height / 2.0 - self.viewStyle.centerUpOffset;
    CGFloat YMaxRetangle = YMinRetangle + sizeRetangle.height;
    CGFloat XRetangleRight = self.frame.size.width - self.viewStyle.xScanRetangleOffset;
    
    CGContextRef context = UIGraphicsGetCurrentContext();
    {//非扫码区域半透明
        const CGFloat *components = CGColorGetComponents(self.viewStyle.notRecoginitonArea.CGColor);
        
        CGFloat red = components[0];
        CGFloat green = components[1];
        CGFloat bule = components[2];
        CGFloat alpha = components[3];
        
        CGContextSetRGBFillColor(context, red, green, bule, alpha);
        
        //填充矩形
        //上
        CGRect rect = CGRectMake(0, 0, self.frame.size.width, YMinRetangle);
        CGContextFillRect(context, rect);
        //左 加了一点点，不然有缝隙，why?
        rect = CGRectMake(0, YMinRetangle - 0.05, self.viewStyle.xScanRetangleOffset, sizeRetangle.height + 0.1);
        CGContextFillRect(context, rect);
        //右
        rect = CGRectMake(XRetangleRight, YMinRetangle - 0.05, self.viewStyle.xScanRetangleOffset, sizeRetangle.height + 0.1);
        CGContextFillRect(context, rect);
        //下
        rect = CGRectMake(0, YMaxRetangle, self.frame.size.width, self.frame.size.height - YMaxRetangle);
        CGContextFillRect(context, rect);
        
        CGContextStrokePath(context);
    }
    if (self.viewStyle.isNeedShowRetangle) {//画矩形框
        CGContextSetStrokeColorWithColor(context, self.viewStyle.colorRetangleLine.CGColor);
        CGContextSetLineWidth(context, 1);
        CGContextAddRect(context, CGRectMake(self.viewStyle.xScanRetangleOffset, YMinRetangle, sizeRetangle.width, sizeRetangle.height));
        CGContextStrokePath(context);
    }
    
    _scanRetangleRect = CGRectMake(self.viewStyle.xScanRetangleOffset, YMinRetangle, sizeRetangle.width, sizeRetangle.height);
    
    //画矩形框4格外围相框角
    //相框角的宽度和高度
    int wAngle = self.viewStyle.photoframeAngleW;
    int hAngle = self.viewStyle.photoframeAngleH;
    
    //4个角的 线的宽度
    CGFloat linewidthAngle = self.viewStyle.photoframeLineW;// 经验参数：6和4
    
    //画扫码矩形以及周边半透明黑色坐标参数
    CGFloat diffAngle = 0.0f;
    
    switch (_viewStyle.photoFrameAngleStyle)
    {
        case LEEScanViewPhotoFrameAngleStyle_Outer:
        {
            diffAngle = linewidthAngle/3;//框外面4个角，与框紧密联系在一起
        }
            break;
        case LEEScanViewPhotoFrameAngleStyle_On:
        {
            diffAngle = 0;
        }
            break;
        case LEEScanViewPhotoFrameAngleStyle_Inner:
        {
            diffAngle = -linewidthAngle/2;
            
        }
            break;
            
        default:
        {
            diffAngle = linewidthAngle/3;
        }
            break;
    }
    
    CGContextSetStrokeColorWithColor(context, _viewStyle.colorAngle.CGColor);
    CGContextSetRGBFillColor(context, 1.0, 1.0, 1.0, 1.0);
    
    // Draw them with a 2.0 stroke width so they are a bit more visible.
    CGContextSetLineWidth(context, linewidthAngle);
    
    
    //
    CGFloat leftX = self.viewStyle.xScanRetangleOffset - diffAngle;
    CGFloat topY = YMinRetangle - diffAngle;
    CGFloat rightX = XRetangleRight + diffAngle;
    CGFloat bottomY = YMaxRetangle + diffAngle;
    
    //左上角水平线
    CGContextMoveToPoint(context, leftX-linewidthAngle/2, topY);
    CGContextAddLineToPoint(context, leftX + wAngle, topY);
    
    //左上角垂直线
    CGContextMoveToPoint(context, leftX, topY-linewidthAngle/2);
    CGContextAddLineToPoint(context, leftX, topY+hAngle);
    
    
    //左下角水平线
    CGContextMoveToPoint(context, leftX-linewidthAngle/2, bottomY);
    CGContextAddLineToPoint(context, leftX + wAngle, bottomY);
    
    //左下角垂直线
    CGContextMoveToPoint(context, leftX, bottomY+linewidthAngle/2);
    CGContextAddLineToPoint(context, leftX, bottomY - hAngle);
    
    
    //右上角水平线
    CGContextMoveToPoint(context, rightX+linewidthAngle/2, topY);
    CGContextAddLineToPoint(context, rightX - wAngle, topY);
    
    //右上角垂直线
    CGContextMoveToPoint(context, rightX, topY-linewidthAngle/2);
    CGContextAddLineToPoint(context, rightX, topY + hAngle);
    
    
    //右下角水平线
    CGContextMoveToPoint(context, rightX+linewidthAngle/2, bottomY);
    CGContextAddLineToPoint(context, rightX - wAngle, bottomY);
    
    //右下角垂直线
    CGContextMoveToPoint(context, rightX, bottomY+linewidthAngle/2);
    CGContextAddLineToPoint(context, rightX, bottomY - hAngle);
    
    CGContextStrokePath(context);
}
```
###### 扫描框有了，开始设置动画：
```swift
- (void)startScanAnimation {
    
    switch (self.viewStyle.animationStyle) {
        case LEEScanViewAnimationStyle_LineMove:
        {
            //线动画
            if (!_scanLineAnimation){
                self.scanLineAnimation = [[LEEScanLineAnimation alloc]init];
            }
            [_scanLineAnimation startAnimatingWithRect:_scanRetangleRect
                                                InView:self
                                                 Image:_viewStyle.animationImage];
        }
            break;
        case LEEScanViewAnimationStyle_NetGrid:
        {
            //网格动画
            if (!_scanNetAnimation)
                self.scanNetAnimation = [[LEEScanNetAnimation alloc]init];
            [_scanNetAnimation startAnimatingWithRect:_scanRetangleRect
                                               InView:self
                                                Image:_viewStyle.animationImage];
        }
            break;
        case LEEScanViewAnimationStyle_LineStill:
        {
            if (!_scanLineStill) {
                
                CGRect stillRect = CGRectMake(_scanRetangleRect.origin.x+20,
                                              _scanRetangleRect.origin.y + _scanRetangleRect.size.height/2,
                                              _scanRetangleRect.size.width-40,
                                              2);
                _scanLineStill = [[UIImageView alloc]initWithFrame:stillRect];
                _scanLineStill.image = _viewStyle.animationImage;
            }
            [self addSubview:_scanLineStill];
        }
            break;
        default:
            break;
    }
}
```
###### 停止动画：
```swift
- (void)stopScanAnimation {
    if (_scanLineAnimation) {
        [_scanLineAnimation stopAnimating];
    }
    
    if (_scanNetAnimation) {
        [_scanNetAnimation stopAnimating];
    }
    
    if (_scanLineStill) {
        [_scanLineStill removeFromSuperview];
    }
}
```
###### 好了，现在有框框，有动画了，再来个散光灯：
```swift
//开关闪光灯
- (void)changeTorch {
    AVCaptureTorchMode torch = _deviceInput.device.torchMode;
    switch (torch) {
        case AVCaptureTorchModeAuto:
            break;
        case AVCaptureTorchModeOff:
            torch = AVCaptureTorchModeOn;
            break;
        case AVCaptureTorchModeOn:
            torch = AVCaptureTorchModeOff;
            break;
        default:
            break;
    }
    [_deviceInput.device lockForConfiguration:nil];
    _deviceInput.device.torchMode = torch;
    [_deviceInput.device unlockForConfiguration];
}
```
###### 闪光灯有了，再来个手势放大镜头：
```swift
//设置镜头缩放
- (void)setVideoScale:(CGFloat)scale {
    //一开始系统默认是1
    [_deviceInput.device lockForConfiguration:nil];
    AVCaptureConnection *videoConnection = [self connectionWithMediaType:AVMediaTypeVideo fromConnections:self.stillImageOutput.connections];
    CGFloat zoom = scale / videoConnection.videoScaleAndCropFactor;
    //videoScaleAndCropFactor cannot be set to a value less than 1.0
    if (scale >= 1) {
        
        videoConnection.videoScaleAndCropFactor = scale;
    }
    [_deviceInput.device unlockForConfiguration];
    if (scale >= 1) {
        
        CGAffineTransform transform = self.videoPreView.transform;
        self.videoPreView.transform = CGAffineTransformScale(transform, zoom, zoom);
    }
}
```
###### 好滴，现在再来个相册中选取照片识别功能：
```swift
#pragma mark : - 打开相册
- (void)openPhotoesLib {
    if ([LEEScanPhotoPermissions photoPermission]) {
        [self openLocalPhoto];
    } else {
        [LEEAlertViewController showWithTitle:@"提示" message:@"请到设置->隐私中开启本程序相册权限"];
    }
}
- (void)openLocalPhoto {
    
    UIImagePickerController *picker = [[UIImagePickerController alloc] init];
    picker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
    picker.delegate = self;
    [self presentViewController:picker animated:YES completion:nil];
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary<NSString *,id> *)info {
    
    [picker dismissViewControllerAnimated:YES completion:nil];
    UIImage *image = [info objectForKey:UIImagePickerControllerEditedImage];
    if (image == nil) {
        image = [info objectForKey:UIImagePickerControllerOriginalImage];
    }
    [LEEScanNative recognizeImage:image success:^(NSArray *result) {
        for (int i = 0; i < result.count; i ++) {
            [self getResultAndDoSomething:result[I]];
        }
        if (result.count == 0) {
            [LEEAlertViewController showWithTitle:@"提示" message:@"未检测到有二维码哦"];
        }
    }];
    
}
- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker {
    [picker dismissViewControllerAnimated:YES completion:nil];
}
```
```swift
+ (void)recognizeImage:(UIImage *)image success:(void(^)(NSArray *))block {
    
    CIDetector *detector = [CIDetector detectorOfType:CIDetectorTypeQRCode context:nil options:@{CIDetectorAccuracy: CIDetectorAccuracyHigh}];
    NSArray *features = [detector featuresInImage:[CIImage imageWithCGImage:image.CGImage]];
    NSMutableArray *mutableStr = [NSMutableArray array];
    for (int i = 0; i < features.count; i ++) {
        CIQRCodeFeature *feature = features[I];
        NSString *scannedResult = feature.messageString;
        [mutableStr addObject:scannedResult];
    }
    block(mutableStr.copy);
}
```
###### ok~ 基本核心功能代码都结束。具体可以下载demo看看哦~!
![项目演示.gif](https://github.com/jiangboLee/SCanDemo/blob/master/demo.gif)


