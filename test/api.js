let currentPage = 1;
let itemsPerPage = 10;
let filteredBooks = [];

let booksDatabase = [];
export const apiBaseUrl = "http://127.0.0.1:8080/api/books"; // 后端 API 地址

// 加载图书列表
export async function loadBooks() {
    const booksContainer = document.getElementById('booksContainer');
    
    // 创建或显示加载消息
    let loadingElement = document.getElementById('loadingMessage');
    if (!loadingElement) {
        loadingElement = document.createElement('div');
        loadingElement.id = 'loadingMessage';
        loadingElement.className = 'text-center py-10 text-gray-500';
        loadingElement.innerHTML = `
            <i class="fas fa-spinner fa-spin text-3xl mb-2"></i>
            <p>正在加载图书数据...</p>
        `;
        booksContainer.innerHTML = '';
        booksContainer.appendChild(loadingElement);
    } else {
        loadingElement.style.display = 'block';
    }
    
    try {
        const response = await fetch(apiBaseUrl);
        if (!response.ok) {
          throw new Error("无法加载图书数据");
        }
        const data = await response.json();
        console.log("加载的图书数据:", data);
        booksDatabase = data.data || [];
        filteredBooks = [...booksDatabase];
        renderBooks();
        updateStats();
      } catch (error) {
        console.error("加载图书失败:", error);
        if (loadingElement) {
          loadingElement.innerHTML = `
            <i class="fas fa-exclamation-triangle text-3xl mb-2 text-red-500"></i>
            <p>加载失败: ${error.message}</p>
          `;
        }
      } finally {
        if (loadingElement) {
          loadingElement.style.display = 'none';
        }
      }      
}

export async function saveBook(bookData, bookId) {
    const method = bookId ? "PUT" : "POST";
    const url = bookId ? `${apiBaseUrl}/${bookId}` : apiBaseUrl;

    const response = await fetch(url, {
        method: method,
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(bookData),
    });

    if (!response.ok) {
        const errorText = await response.text();
        throw new Error(errorText);
    }

    return await response.json();
}

export async function confirmDelete(bookId) {
    const url = `${apiBaseUrl}/${bookId}`;
    const response = await fetch(url, { method: "DELETE" });

    if (!response.ok) {
        const errorText = await response.text();
        throw new Error(errorText);
    }

    return true;
}