// sparse bit set for 32bit
// splits space to 6 bit blocks
// allocates blocks on demand
class SparseSet
{
public:
	void Init(DWORD nTotal);
	void Set(size_t idx);
	void Get(size_t idx);
	void GetTotal();

private:
	struct Block
	{
		DWORD Total;
		DWORD Set;
		bool AllSet;
		DWORD *pData;
	};
	DWORD _Total;
	DWORD _Set;
	std::vector<Block> _Data;
};

