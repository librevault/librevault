use sea_orm_migration::prelude::*;

#[derive(DeriveMigrationName)]
pub struct Migration;

#[async_trait::async_trait]
impl MigrationTrait for Migration {
    async fn up(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        manager
            .create_table(
                Table::create()
                    .table(Materialized::Table)
                    .col(
                        ColumnDef::new(Materialized::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(ColumnDef::new(Materialized::Bucket).binary().not_null())
                    .col(
                        ColumnDef::new(Materialized::ChunkMdHash)
                            .binary()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(Materialized::PlaintextHash)
                            .binary()
                            .not_null(),
                    )
                    .col(ColumnDef::new(Materialized::Path).binary().not_null())
                    .col(
                        ColumnDef::new(Materialized::Offset)
                            .big_integer()
                            .not_null(),
                    )
                    .to_owned(),
            )
            .await?;
        manager
            .create_index(
                Index::create()
                    .table(Materialized::Table)
                    .name("materialized_bucket_chunk_md_hash_index")
                    .col(Materialized::Bucket)
                    .col(Materialized::ChunkMdHash)
                    .to_owned(),
            )
            .await?;
        manager
            .create_index(
                Index::create()
                    .table(Materialized::Table)
                    .name("materialized_plaintext_hash_index")
                    .col(Materialized::PlaintextHash)
                    .to_owned(),
            )
            .await
    }

    async fn down(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        manager
            .drop_table(Table::drop().table(Materialized::Table).to_owned())
            .await
    }
}

/// Learn more at https://docs.rs/sea-query#iden
#[derive(Iden)]
enum Materialized {
    Table,
    Id,
    Bucket,
    ChunkMdHash,
    PlaintextHash,
    Path,
    Offset,
}
