import {
    openAddBookModal,
    editBook,
    closeModal,
    closeDeleteModal
  } from '../modal';
  
  // 模拟DOM元素
  const mockElements = {
    bookForm: {
      reset: jest.fn()
    },
    bookId: {
      value: ''
    },
    modalTitle: {
      textContent: ''
    },
    saveButtonText: {
      textContent: ''
    },
    bookModal: {
      classList: {
        remove: jest.fn(),
        add: jest.fn()
      }
    },
    deleteModal: {
      classList: {
        remove: jest.fn(),
        add: jest.fn()
      }
    },
    mainContent: {
      classList: {
        add: jest.fn(),
        remove: jest.fn()
      }
    },
    // 表单字段
    title: { value: '' },
    author: { value: '' },
    isbn: { value: '' },
    category: { value: '' },
    status: { value: '' },
    publishDate: { value: '' },
    description: { value: '' }
  };
  
  // 设置document.getElementById模拟
  beforeAll(() => {
    global.document.getElementById = jest.fn((id) => {
      return mockElements[id] || null;
    });
  });
  
  // 在每个测试前重置所有模拟
  beforeEach(() => {
    jest.clearAllMocks();
    
    // 重置模拟元素的状态
    mockElements.bookId.value = '';
    mockElements.modalTitle.textContent = '';
    mockElements.saveButtonText.textContent = '';
  });
  
  describe('Modal Functions', () => {
    describe('openAddBookModal', () => {
      it('should reset form and open modal for adding new book', () => {
        openAddBookModal();
  
        // 验证表单重置
        expect(mockElements.bookForm.reset).toHaveBeenCalled();
        expect(mockElements.bookId.value).toBe('');
  
        // 验证标题和按钮文本
        expect(mockElements.modalTitle.textContent).toBe('添加新图书');
        expect(mockElements.saveButtonText.textContent).toBe('保存');
  
        // 验证模态框显示
        expect(mockElements.bookModal.classList.remove)
          .toHaveBeenCalledWith('opacity-0', 'invisible');
        expect(mockElements.bookModal.classList.add)
          .toHaveBeenCalledWith('opacity-100', 'visible');
  
        // 验证背景模糊
        expect(mockElements.mainContent.classList.add)
          .toHaveBeenCalledWith('blur-background');
      });
    });
  
    describe('editBook', () => {
      const testBook = {
        id: '123',
        title: 'Test Book',
        author: 'Test Author',
        isbn: '1234567890',
        category: 'Fiction',
        status: 'Available',
        publishDate: '2023-01-01',
        description: 'Test description'
      };
  
      it('should populate form and open modal for editing book', () => {
        editBook(testBook);
  
        // 验证表单字段填充
        expect(mockElements.bookId.value).toBe(testBook.id);
        expect(mockElements.title.value).toBe(testBook.title);
        expect(mockElements.author.value).toBe(testBook.author);
        expect(mockElements.isbn.value).toBe(testBook.isbn);
        expect(mockElements.category.value).toBe(testBook.category);
        expect(mockElements.status.value).toBe(testBook.status);
        expect(mockElements.publishDate.value).toBe(testBook.publishDate);
        expect(mockElements.description.value).toBe(testBook.description);
  
        // 验证标题和按钮文本
        expect(mockElements.modalTitle.textContent).toBe('编辑图书信息');
        expect(mockElements.saveButtonText.textContent).toBe('更新');
  
        // 验证模态框显示
        expect(mockElements.bookModal.classList.remove)
          .toHaveBeenCalledWith('opacity-0', 'invisible');
        expect(mockElements.bookModal.classList.add)
          .toHaveBeenCalledWith('opacity-100', 'visible');
  
        // 验证背景模糊
        expect(mockElements.mainContent.classList.add)
          .toHaveBeenCalledWith('blur-background');
      });
  
      it('should handle empty description', () => {
        const bookWithoutDescription = { ...testBook, description: undefined };
        editBook(bookWithoutDescription);
        
        expect(mockElements.description.value).toBe('');
      });
    });
  
    describe('closeModal', () => {
      it('should hide the book modal and remove blur', () => {
        closeModal();
  
        // 验证模态框隐藏
        expect(mockElements.bookModal.classList.remove)
          .toHaveBeenCalledWith('opacity-100', 'visible');
        expect(mockElements.bookModal.classList.add)
          .toHaveBeenCalledWith('opacity-0', 'invisible');
  
        // 验证背景模糊移除
        expect(mockElements.mainContent.classList.remove)
          .toHaveBeenCalledWith('blur-background');
      });
    });
  
    describe('closeDeleteModal', () => {
      it('should hide the delete modal and remove blur', () => {
        closeDeleteModal();
  
        // 验证删除模态框隐藏
        expect(mockElements.deleteModal.classList.remove)
          .toHaveBeenCalledWith('opacity-100', 'visible');
        expect(mockElements.deleteModal.classList.add)
          .toHaveBeenCalledWith('opacity-0', 'invisible');
  
        // 验证背景模糊移除
        expect(mockElements.mainContent.classList.remove)
          .toHaveBeenCalledWith('blur-background');
      });
    });
  });