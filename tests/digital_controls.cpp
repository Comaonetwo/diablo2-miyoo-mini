// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamepad_map.hpp>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
using namespace d2d::client::pad;
#define check(yes) do{if(!(yes))throw std::runtime_error("digital control check failed at line "+std::to_string(__LINE__));}while(false)
int main()try{
    {Mapper slow;Sample q;q.digital=true;q.held[std::size_t(Button::Back)]=true;
     slow.step(q,Context::Game,.1f,.8f); // time before the press does not count
     slow.step(q,Context::Game,.1f,.35f);slow.step(q,Context::Game,.1f,.35f);
     q.held[std::size_t(Button::DpadUp)]=true;
     check(slow.step(q,Context::Game,.1f,.35f).keys.empty());}
    for(auto context:{Context::Game,Context::Panel,Context::Shop}){Mapper cursor;Sample q;q.digital=true;q.held[std::size_t(Button::Back)]=true;
     for(int i=0;i<5;++i)cursor.step(q,context,.3f);
     for(auto b:{Button::DpadUp,Button::DpadDown,Button::DpadLeft,Button::DpadRight}){
      q.held[std::size_t(b)]=true;auto out=cursor.step(q,context,.04f);
      check(out.keys.empty()&&out.cursor_mode&&!out.move_active&&(out.cursor_dx!=0||out.cursor_dy!=0));
      q.held[std::size_t(b)]=false;cursor.step(q,context,.04f);
     }
     q.held.fill(false);check(cursor.step(q,context,.04f).keys.empty());
     q.held[std::size_t(Button::Back)]=true;q.held[std::size_t(Button::DpadUp)]=true;
     auto out=cursor.step(q,context,.04f);check(out.keys.size()==1&&out.keys[0]==Key::SkillTree);}
    {Mapper sort;Sample q;q.digital=true;q.held[std::size_t(Button::Back)]=true;sort.step(q,Context::Panel,.04f);q.held[std::size_t(Button::Start)]=true;auto a=sort.step(q,Context::Panel,.04f);check(a.quick_sort&&!a.large_text_toggle&&a.keys.empty()&&!a.click_press);}
    {Mapper shop;Sample q;q.digital=true;q.held[std::size_t(Button::Back)]=true;shop.step(q,Context::Shop,.04f);q.held[std::size_t(Button::West)]=true;auto out=shop.step(q,Context::Shop,.04f);check(out.trade_press&&out.fill_trade&&!out.quick_belt&&!out.quick_drop&&out.keys.empty());q.held.fill(false);out=shop.step(q,Context::Shop,.04f);check(out.keys.empty());}
    Mapper m;Sample s;s.digital=true;
    {Mapper quick;Sample q;q.digital=true;q.held[std::size_t(Button::Back)]=true;quick.step(q,Context::Panel,.04f);
     q.held[std::size_t(Button::West)]=true;auto out=quick.step(q,Context::Panel,.04f);check(out.quick_belt&&!out.rclick_press&&!out.interact_press);
     q.held.fill(false);out=quick.step(q,Context::Panel,.04f);check(std::find(out.keys.begin(),out.keys.end(),Key::Automap)==out.keys.end());}

    s.held[std::size_t(Button::DpadRight)]=true;s.held[std::size_t(Button::DpadUp)]=true;
    auto a=m.step(s,Context::Game,.04f);check(a.move_active&&a.move_x>0&&a.move_y<0&&a.keys.empty());
    a=m.step({},Context::Game,.04f);check(!a.move_active);
    a=m.step(s,Context::Panel,.04f);check(!a.move_active&&a.cursor_dx==0&&a.cursor_dy==0&&a.snap_x==1&&a.snap_y==-1);
    s.held.fill(false);m.step(s,Context::Game,.04f);s.held[std::size_t(Button::West)]=true;
    a=m.step(s,Context::Game,.04f);check(a.interact_press&&!a.click_press&&!a.rclick_press);
    check(!m.step(s,Context::Game,.04f).interact_press);
    s.held.fill(false);m.step(s,Context::Game,.04f);s.held[std::size_t(Button::North)]=true;
    a=m.step(s,Context::Game,.04f);check(a.rclick_press&&a.keys.empty());
    s.held.fill(false);m.step(s,Context::Panel,.04f);s.held[std::size_t(Button::East)]=true;
    a=m.step(s,Context::Panel,.04f);check(std::find(a.keys.begin(),a.keys.end(),Key::Escape)!=a.keys.end());
    s.held.fill(false);m.step(s,Context::Game,.04f);s.held[std::size_t(Button::LeftShoulder)]=true;
    a=m.step(s,Context::Game,.04f);check(std::find(a.keys.begin(),a.keys.end(),Key::HealPotion)!=a.keys.end());
    s.held.fill(false);m.step(s,Context::Title,.04f);
    s.held[std::size_t(Button::DpadRight)]=true;
    a=m.step(s,Context::Title,.04f);check(a.cursor_dx==0&&!a.move_active&&a.snap_x==1&&a.keys.empty());
    a=m.step(s,Context::Menu,.04f);check(a.cursor_dx==0&&!a.move_active);
    s.held.fill(false);m.step(s,Context::Menu,.04f);s.held[std::size_t(Button::South)]=true;
    a=m.step(s,Context::Menu,.04f);check(a.click_press&&a.click_down);
    s.held.fill(false);a=m.step(s,Context::Menu,.04f);check(a.click_release);
    s.held.fill(false);m.step(s,Context::Game,.04f);
    s.held[std::size_t(Button::Back)]=true;s.held[std::size_t(Button::DpadRight)]=true;
    a=m.step(s,Context::Game,.04f);check(a.cursor_mode&&a.cursor_dx>0&&!a.move_active&&a.keys.size()==1&&a.keys[0]==Key::Mercenary);
    s.held[std::size_t(Button::Start)]=true;a=m.step(s,Context::Game,.04f);check(a.large_text_toggle&&a.keys.empty());
    s.held.fill(false);a=m.step(s,Context::Game,.04f);check(a.keys.empty());
    s.held[std::size_t(Button::Back)]=true;check(m.step(s,Context::Game,.04f).keys.empty());
    s.held.fill(false);a=m.step(s,Context::Game,.04f);check(a.keys.size()==1&&a.keys[0]==Key::Automap);
    s.held.fill(false);m.step(s,Context::Panel,.04f);s.held[std::size_t(Button::West)]=true;
    a=m.step(s,Context::Panel,.04f);check(a.rclick_press&&!a.interact_press&&!a.click_press);
    s.held.fill(false);m.step(s,Context::Game,.04f);s.held[std::size_t(Button::Back)]=true;s.held[std::size_t(Button::LeftShoulder)]=true;
    a=m.step(s,Context::Game,.04f);check(std::find(a.keys.begin(),a.keys.end(),Key::Belt3)!=a.keys.end());
    s.held.fill(false);a=m.step(s,Context::Game,.04f);check(a.keys.empty());
    s.held.fill(false);m.step(s,Context::Game,.04f);s.held[std::size_t(Button::Back)]=true;s.held[std::size_t(Button::LeftStick)]=true;
    a=m.step(s,Context::Game,.04f);check(a.keys.empty());
    s.held.fill(false);m.step(s,Context::Game,.04f);s.held[std::size_t(Button::Back)]=true;s.held[std::size_t(Button::DpadUp)]=true;
    a=m.step(s,Context::Game,.04f);check(a.keys.size()==1&&a.keys[0]==Key::SkillTree&&!a.move_active);
    s.held.fill(false);a=m.step(s,Context::Game,.04f);check(a.keys.empty());
    for (const auto [button,key] : {std::pair{Button::DpadDown,Key::QuestLog},std::pair{Button::DpadLeft,Key::ShowBelt},std::pair{Button::DpadRight,Key::Mercenary}}) {
        Mapper chord; Sample q;q.digital=true;q.held[std::size_t(Button::Back)]=true;q.held[std::size_t(button)]=true;
        auto out=chord.step(q,Context::Game,.04f);check(out.keys.size()==1&&out.keys[0]==key&&!out.move_active);
        q.held.fill(false);check(chord.step(q,Context::Game,.04f).keys.empty());
    }
    s.held.fill(false);m.step(s,Context::Game,.04f);s.held[std::size_t(Button::Back)]=true;s.held[std::size_t(Button::North)]=true;
    a=m.step(s,Context::Game,.04f);check(a.keys.size()==1&&a.keys[0]==Key::ToggleRun&&!a.rclick_press);
    s.held.fill(false);check(m.step(s,Context::Game,.04f).keys.empty());
    s.held.fill(false);m.step(s,Context::Game,.04f);s.held[std::size_t(Button::Back)]=true;s.held[std::size_t(Button::West)]=true;
    a=m.step(s,Context::Game,.04f);check(a.keys.size()==1&&a.keys[0]==Key::SwapWeapons&&!a.interact_press);
    s.held.fill(false);check(m.step(s,Context::Game,.04f).keys.empty());
    s.held[std::size_t(Button::Back)]=true;s.held[std::size_t(Button::North)]=true;
    a=m.step(s,Context::Panel,.04f);check(a.quick_drop&&a.keys.empty()&&!a.rclick_press&&!a.click_press);
    s.held.fill(false);check(m.step(s,Context::Panel,.04f).keys.empty());
    check(visible_drop(-1.f,-4.9f,false)&&!visible_drop(5.5f,0,false));
    check(!visible_drop(-1.f,-4.9f,true)&&visible_drop(0,0,true));
    check(visible_drop(1.75f,-1.75f,1)&&!visible_drop(1.75f,-1.75f,2)&&visible_drop(0,0,2));
    std::puts("PASS: digital movement, diagonal, stop, panel focus, separate loot/cast, cancel and potion");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
