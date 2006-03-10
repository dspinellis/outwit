#include <stdio.h>
#include <windows.h>
#include <olectl.h>
#include <ole2.h>


// Function LoadAnImage: accepts a file name and returns a HBITMAP.
// On error, it returns 0.
// http://www.codeguru.com/cpp/g-m/bitmap/article.php/c4935/
extern "C" HBITMAP
image_load(FILE *fp)
   {
   // Use IPicture stuff to use JPG / GIF files
   IPicture* p;
   IStream* s;
   IPersistStream* ps;
   HGLOBAL hG;
   void* pp;
   int fs;

   fseek(fp,0,SEEK_END);
   fs = ftell(fp);
   fseek(fp,0,SEEK_SET);
   hG = GlobalAlloc(GPTR,fs);
   if (!hG) {
      fclose(fp);
      return NULL;
      }
   pp = (void*)hG;
   fread(pp,1,fs,fp);
   fclose(fp);

LPSTREAM pstm = NULL;
HRESULT hr = CreateStreamOnHGlobal(hG, TRUE, &pstm);
   if (!pstm) {
      GlobalFree(hG);
      return NULL;
      }

   OleLoadPicture(pstm,0,0,IID_IPicture,(void**)&p);

   if (!p) {
      pstm->Release();
      GlobalFree(hG);
      return NULL;
      }
   pstm->Release();
   GlobalFree(hG);

   HBITMAP hB = 0;
   p->get_Handle((unsigned int*)&hB);

   // Copy the image. Necessary, because upon p's release,
   // the handle is destroyed.
   HBITMAP hBB = (HBITMAP)CopyImage(hB,IMAGE_BITMAP,0,0, LR_COPYRETURNORG);

   p->Release();
   return hBB;
   }
