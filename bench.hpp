class benchmark {
	private:
		std::atomic<bool> start_;
		std::atomic<bool> stop_;

		void internal_thread(int idx);

	protected:
		bool should_continue();
		virtual void before() {}
		virtual void after() {}
		virtual void thread(int idx) = 0;

	public:
		void run(int seconds, int threads);
};

