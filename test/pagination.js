let filteredBooks = [];
let currentPage = 1;
let itemsPerPage = 10;

export function previousPage(renderBooks) {
    if (currentPage > 1) {
        currentPage--;
        renderBooks();
    }
}

export function nextPage(renderBooks, totalItems) {
    const totalPages = Math.ceil(totalItems / itemsPerPage);
    if (currentPage < totalPages) {
        currentPage++;
        renderBooks();
    }
}

export function changeItemsPerPage(newItemsPerPage, renderBooks) {
    itemsPerPage = newItemsPerPage;
    currentPage = 1;
    renderBooks();
}