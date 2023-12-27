# hoonzip
압축 해제 프로그램입니다. ZIP 포맷만 지원하고 지원하는 압축 알고리즘은 DEFLATE 하나입니다.
## 사용방법

release에 미리 컴파일된 바이너리와 테스트용으로 사용한 zip 파일이 준비되어 있습니다. release에서 다운받은 파일을 사용하거나 소스코드를 직접 컴파일하고 본인이 준비한 zip파일로 테스트 해보세요

### 기본 사용법
hoonzip zipfile

![image](https://github.com/zlwhro/hoonzip/assets/113174616/befc3daf-e0aa-4e74-8972-b98537395f9e)

### 파일 리스팅

hoonzip -l zipfile

![image](https://github.com/zlwhro/hoonzip/assets/113174616/c79354df-27b2-4d90-80ff-710ca0a49332)


-l 옵션은 저장된 파일의 리스트를 출력합니다. 압축을 해제할때 -l 옵션으로 zip 파일이 DEFLATE 압축을 쓰는지 확인하고 압축을 해제해주세요

### 저장 경로 지정
hoonzip -d exdir zipfile

![image](https://github.com/zlwhro/hoonzip/assets/113174616/93f5edac-c90a-4015-84d9-102156bf47b7)


-d 옵션으로 압축 해제된 파일이 저장된 경로를 지정할 수 있습니다.



