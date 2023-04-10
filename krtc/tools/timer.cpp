#include "timer.h"

CTimer::CTimer(unsigned int milliseconds, bool repeat, CTimer::OnTask func) :
	milliseconds_(milliseconds),
	repeat_(repeat),
	func_(func) {
}

CTimer::~CTimer()
{}

// ��������
void CTimer::Start()
{
	if (milliseconds_ == 0 || milliseconds_ == static_cast<unsigned int>(-1)) // ���ʱ��Ϊ0��Ĭ����Чֵ��ֱ�ӷ���
	{
		return;
	}
	exit_.store(false);
	immediately_run_.store(false);
	thread_ = std::thread(std::bind(&CTimer::Run, this));
}

void CTimer::Stop()
{
	exit_.store(true);
	cond_.notify_all(); // �����߳�
	if (thread_.joinable())
	{
		thread_.join();
	}
}

void CTimer::SetExit(bool b_exit)
{
	exit_.store(b_exit);
}

void CTimer::Run()
{
	while (!exit_.load())
	{
		{
			std::unique_lock<std::mutex> locker(mutex_);

			// ����Ǳ����ѵģ���Ҫ�ж�������ȷ���ǲ�����ٻ���
			// wait_for�ǵȴ�����������������������������ʱ����ʱ���������ִ��
			cond_.wait_for(locker, std::chrono::milliseconds(milliseconds_), [this]()
				{ return exit_.load(); }
			);
		}

		if (exit_.load()) // ��У�����˳���ʶ
		{
			return;
		}

		if (func_)
		{
			func_();

			if (!repeat_) {
				SetExit(true);
			}
		}
	}
}
