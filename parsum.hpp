class parsum {
	private:
		struct node {
		};

		struct will_element {
		};

		const int T_;
		std::unique_ptr<node[]> tree_;
		std::unique_ptr<will_element[][]> wills_;

	public:
		parsum(int T) : T_(T) {/*fixme*/}
	
