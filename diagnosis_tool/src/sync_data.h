class SyncData {

	public:
		SyncData(){
			instcount = 0;
			inst_mem_access = 0; 	
			critical_sections = 0;
		}
		~SyncData(){}
		unsigned int instcount;
		unsigned int inst_mem_access; // no of instruction that do mem access
		unsigned int critical_sections;

};
