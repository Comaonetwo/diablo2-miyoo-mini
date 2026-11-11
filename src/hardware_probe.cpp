// SPDX-License-Identifier: GPL-3.0-or-later
#include "data_paths.hpp"
#include <dc6.hpp>
#include <font.hpp>
#include <palette.hpp>
#include <chrono>
#include <csignal>
#include <thread>
#include <cstdio>
#include <sys/resource.h>
#ifdef D2D_MIYOO
#include "miyoo_platform.hpp"
#include <linux/input.h>
#endif
namespace { volatile std::sig_atomic_t stopping=0; void stop(int){stopping=1;} }
int main(int argc,char** argv)try{
    if(argc!=3&&argc!=4){std::fprintf(stderr,"Usage: hardware-probe <data-dir> <LoD-patch> [preview.ppm]\n");return 2;}
    std::setvbuf(stdout,nullptr,_IONBF,0); std::signal(SIGINT,stop);std::signal(SIGTERM,stop);
    auto data=d2mini::open_data(argv[1],argv[2]);
    const d2d::dc6::Sprite bg(data.read(R"(data\global\ui\FrontEnd\gameselectscreenEXP.dc6)"));
    const d2d::dc6::Sprite hero(data.read(R"(data\global\ui\FrontEnd\barbarian\banu1.dc6)"));
    const d2d::palette::Palette sky(data.read(R"(data\global\palette\sky\pal.dat)")), fire(data.read(R"(data\global\palette\fechar\pal.dat)"));
    const d2d::font::Font font(data.read(R"(data\local\FONT\LATIN\font16.tbl)"),d2d::dc6::Sprite(data.read(R"(data\local\FONT\LATIN\font16.dc6)")));
    std::vector<std::uint8_t> fb(800*600*4);int hx=400,hy=430;unsigned tick=0;
    const auto draw=[&](const d2d::dc6::Frame& f,const d2d::palette::Palette& pal,int left,int top){
        for(int y=0;y<int(f.height);++y)for(int x=0;x<int(f.width);++x){
            const int xx=left+x,yy=top+y;if(xx<0||yy<0||xx>=800||yy>=600)continue;
            const auto i=f.pixels[std::size_t(y)*f.width+x];if(!i)continue;const auto c=pal[i];
            auto* p=fb.data()+(yy*800+xx)*4;p[0]=c.r;p[1]=c.g;p[2]=c.b;p[3]=255;
        }
    };
#ifdef D2D_MIYOO
    std::unique_ptr<d2mini::Platform> device;
    if(argc==3)device=std::make_unique<d2mini::Platform>();
#endif
    const auto start=std::chrono::steady_clock::now();
    while(!stopping){
#ifdef D2D_MIYOO
        if(device){device->poll();if(device->exiting())break;
            hx+=4*(int(device->held(KEY_RIGHT))-int(device->held(KEY_LEFT)));
            hy+=4*(int(device->held(KEY_DOWN))-int(device->held(KEY_UP)));
            hx=std::clamp(hx,100,700);hy=std::clamp(hy,200,500);
        }
#endif
        std::fill(fb.begin(),fb.end(),0);int x=0,y=0;
        for(const auto& f:bg.frames()){draw(f,sky,x,y);x+=int(f.width);if(x>=800){x=0;y+=int(f.height);}}
        const auto& frame=hero.frame(0,(tick/3)%hero.frames_per_direction());
        draw(frame,fire,hx+frame.offset_x,hy+frame.offset_y-int(frame.height)+1);
        for(int row=0;row<100;++row)for(int col=0;col<800;++col){auto* p=fb.data()+(row*800+col)*4;p[0]/=3;p[1]/=3;p[2]/=3;}
        font.draw(fb,800,600,sky,20,12,"DIABLO II - HARDWARE TEST");
        font.draw(fb,800,600,sky,20,40,"D-PAD: MOVE SPRITE. MENU + SELECT: EXIT.");
        font.draw(fb,800,600,sky,20,68,"DISPLAY / KEYS / MPQ TEST - THIS IS NOT GAMEPLAY.");
        ++tick;
        if(argc==4){std::ofstream out(argv[3],std::ios::binary);out<<"P6\n800 600\n255\n";for(std::size_t i=0;i<fb.size();i+=4)out.write(reinterpret_cast<const char*>(fb.data()+i),3);if(!out)throw std::runtime_error("Preview write failed");break;}
#ifdef D2D_MIYOO
        device->present(fb,800,600);
        if(tick%125==0){rusage r{};getrusage(RUSAGE_SELF,&r);std::printf("frames=%u elapsed_ms=%lld peak_rss_kib=%ld\n",tick,(long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count(),r.ru_maxrss);}
#else
        std::fprintf(stderr,"Use a preview filename on desktop\n");return 2;
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    std::puts("Hardware test exited normally");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
