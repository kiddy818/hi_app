#ifndef dev_vo_bt1120_include_h
#define dev_vo_bt1120_include_h

#include "dev_vo.h"
#include "dev_vi.h"

namespace hisilicon{namespace dev{

    class vo_bt1120
        :public vo
    {
        public:
            vo_bt1120(int w,int h,int fr,int vo_dev,std::shared_ptr<vi> vi_ptr);

            virtual ~vo_bt1120();

            bool start() override;

            void stop() override;

        private:
            std::shared_ptr<vi> m_vi_ptr;
    };

}}//namespace

#endif
