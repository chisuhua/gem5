// typedef int _cuda_device_id;
struct _cuda_device_id {
	_cuda_device_id(gpgpu_sim* gpu) {m_id = 0; m_next = NULL; m_gpgpu=gpu;}
	struct _cuda_device_id *next() { return m_next; }
	// unsigned num_shader() const { return m_gpgpu->get_config().num_shader(); }
	int num_devices() const {
		if( m_next == NULL ) return 1;
		else return 1 + m_next->num_devices();
	}
	struct _cuda_device_id *get_device( unsigned n )
	{
		assert( n < (unsigned)num_devices() );
		struct _cuda_device_id *p=this;
		for(unsigned i=0; i<n; i++)
			p = p->m_next;
		return p;
	}
	const struct cudaDeviceProp *get_prop() const
	{
        if (prop == NULL)
            gem5cudaGetDeviceProperties(prop, *m_id);
        return prop;
		// return m_gpgpu->get_prop();
	}
	int get_id() const { return *m_id; }
	gpgpu_sim *get_gpgpu() { return m_gpgpu; }

    // hardcode 70
	int get_forced_max_capability() const { 70; }
	bool convert_to_ptxplus() const { false; }

    // in original it is m_ptx_use_cuobjdump in abstract_hardware_model
	bool use_cuobjdump() const { true; }

	// gpgpu_sim *get_gpgpu() { return m_gpgpu; }
private:
	int* m_id;
    struct cudaDeviceProp* prop;
	class gpgpu_sim *m_gpgpu;
	struct _cuda_device_id *m_next;
};


struct CUctx_st {
	CUctx_st( _cuda_device_id *gpu )
	{
		m_gpu = gpu;
		m_binary_info.cmem = 0;
		m_binary_info.gmem = 0;
	}

	_cuda_device_id *get_device() { return m_gpu; }

	void add_binary( symbol_table *symtab, unsigned fat_cubin_handle )
	{
		m_code[fat_cubin_handle] = symtab;
		m_last_fat_cubin_handle = fat_cubin_handle;
	}

	void add_ptxinfo( const char *deviceFun, const struct gpgpu_ptx_sim_info &info )
	{
        /*
		// symbol *s = m_code[m_last_fat_cubin_handle]->lookup(deviceFun);
		symbol *s = gem5gpu_symbol_lookup(m_code[m_last_fat_cubin_handle], deviceFun);
		assert( s != NULL );
		function_info *f = s->get_pc();
        */
		function_info *f = gem5gpu_symbol_get_function(m_code[m_last_fat_cubin_handle], deviceFun);
		assert( f != NULL );
		f->set_kernel_info(info);
	}

	void add_ptxinfo( const struct gpgpu_ptx_sim_info &info )
	{
		m_binary_info = info;
	}

	void register_function( unsigned fat_cubin_handle, const char *hostFun, const char *deviceFun )
	{
		if( m_code.find(fat_cubin_handle) != m_code.end() ) {
			// symbol *s = m_code[fat_cubin_handle]->lookup(deviceFun);
            /*
			symbol *s = gem5gpu_symbol_lookup(m_code[fat_cubin_handle], deviceFun);
			if(s != NULL) {
				function_info *f = s->get_pc();
				assert( f != NULL );
				m_kernel_lookup[hostFun] = f;
			}
			else {
				printf("Warning: cannot find deviceFun %s\n", deviceFun);
				m_kernel_lookup[hostFun] = NULL;
			}
            */
		    function_info *f = gem5gpu_symbol_get_function(m_code[m_last_fat_cubin_handle], deviceFun);
			m_kernel_lookup[hostFun] = f;
	        //		assert( s != NULL );
        	//		function_info *f = s->get_pc();
        	//		assert( f != NULL );
        	//		m_kernel_lookup[hostFun] = f;
		} else {
			m_kernel_lookup[hostFun] = NULL;
		}
        gem5cudaRegisterFunction((void*)fat_cubin_handle, hostFun, deviceFun);
	}

    void register_hostFun_function( const char*hostFun, function_info* f){
        m_kernel_lookup[hostFun] = f;
    }

	function_info *get_kernel(const char *hostFun)
	{
		std::map<const void*,function_info*>::iterator i=m_kernel_lookup.find(hostFun);
		assert( i != m_kernel_lookup.end() );
		return i->second;
	}

private:
	_cuda_device_id *m_gpu; // selected gpu
	std::map<unsigned,symbol_table*> m_code; // fat binary handle => global symbol table
	unsigned m_last_fat_cubin_handle;
	std::map<const void*,function_info*> m_kernel_lookup; // unique id (CUDA app function address) => kernel entry point
	struct gpgpu_ptx_sim_info m_binary_info;

};

class kernel_config {
public:
	kernel_config( dim3 GridDim, dim3 BlockDim, size_t sharedMem, struct libcuda::CUstream_st *stream )
	{
		m_GridDim=GridDim;
		m_BlockDim=BlockDim;
		m_sharedMem=sharedMem;
		m_stream = stream;
	}
	kernel_config()
	{
		m_GridDim=dim3(-1,-1,-1);
		m_BlockDim=dim3(-1,-1,-1);
		m_sharedMem=0;
		m_stream =NULL;
	}
	void set_arg( const void *arg, size_t size, size_t offset )
	{
		m_args.push_front( gpgpu_ptx_sim_arg(arg,size,offset) );
	}
	dim3 grid_dim() const { return m_GridDim; }
	dim3 block_dim() const { return m_BlockDim; }
	void set_grid_dim(dim3 *d) { m_GridDim = *d; }
	void set_block_dim(dim3 *d) { m_BlockDim = *d; }
	gpgpu_ptx_sim_arg_list_t get_args() { return m_args; }
	struct libcuda::CUstream_st *get_stream() { return m_stream; }

private:
	dim3 m_GridDim;
	dim3 m_BlockDim;
	size_t m_sharedMem;
	struct libcuda::CUstream_st *m_stream;
	gpgpu_ptx_sim_arg_list_t m_args;
};

